/*
A small wrapper utility to load indicators and put them as menu items
into the gnome-panel using it's applet interface.

Copyright 2010 Canonical Ltd.

Authors:
    Conor Curran <conor.curran@canonical.com>

This program is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License version 3, as published
by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranties of
MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include <pulse/glib-mainloop.h>
#include <pulse/error.h>
#include <pulse/gccmacro.h>

#include "pulse-manager.h"
#include "dbus-menu-manager.h"

static GHashTable *sink_hash = NULL;
static SoundServiceDbus *dbus_service = NULL;
static gint DEFAULT_SINK_INDEX = -1;
static gboolean pa_server_available = FALSE;
// PA related
static pa_context *pulse_context = NULL;
static pa_glib_mainloop *pa_main_loop = NULL;
static void context_state_callback(pa_context *c, void *userdata);
static void pulse_sink_info_callback(pa_context *c, const pa_sink_info *sink_info, int eol, void *userdata);
static void context_success_callback(pa_context *c, int success, void *userdata);
static void pulse_sink_input_info_callback(pa_context *c, const pa_sink_input_info *info, int eol, void *userdata);
static void pulse_server_info_callback(pa_context *c, const pa_server_info *info, void *userdata);
static void update_sink_info(pa_context *c, const pa_sink_info *info, int eol, void *userdata);
static void destroy_sink_info(void *value);
static gboolean determine_sink_availability();
static void reconnect_to_pulse();

static gboolean has_volume_changed(const pa_sink_info* new_sink, sink_info* cached_sink);
static pa_cvolume construct_mono_volume(const pa_cvolume* vol);

/**
Future Refactoring notes
 - rewrite in vala.
 - make sure all state is kept in the service for volume icon switching.
**/

/**
Entry point
**/
void establish_pulse_activities(SoundServiceDbus *service)
{
  dbus_service = service;
  pa_main_loop = pa_glib_mainloop_new(g_main_context_default());
  g_assert(pa_main_loop);
  pulse_context = pa_context_new(pa_glib_mainloop_get_api(pa_main_loop), "ayatana.indicator.sound");
  g_assert(pulse_context);

  sink_hash = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, destroy_sink_info);

  // Establish event callback registration
  pa_context_set_state_callback(pulse_context, context_state_callback, NULL);
  dbus_menu_manager_update_pa_state(FALSE, FALSE, FALSE, 0);
  pa_context_connect(pulse_context, NULL, PA_CONTEXT_NOFAIL, NULL);
}

/**
get_context()
Needed for testing - bah!
**/
pa_context* get_context()
{
  return pulse_context;
}

/**
close_pulse_activites()
Gracefully close our connection with the Pulse async library.
**/
void close_pulse_activites()
{
  if (pulse_context != NULL) {
    /*        g_debug("freeing the pulse context");*/
 		pa_context_unref(pulse_context);
    pulse_context = NULL;
  }
  g_hash_table_destroy(sink_hash);
  pa_glib_mainloop_free(pa_main_loop);
  pa_main_loop = NULL;
  /*    g_debug("I just closed communication with Pulse");*/
}

/**
reconnect_to_pulse()
In the event of Pulseaudio flapping in the wind handle gracefully without
memory leaks !
*/
static void reconnect_to_pulse()
{
  // reset
  if (pulse_context != NULL) {
    g_debug("freeing the pulse context");
    pa_context_unref(pulse_context);
    pulse_context = NULL;
  }

  if (sink_hash != NULL) {
    g_hash_table_destroy(sink_hash);
    sink_hash = NULL;
  }

  // reconnect
  pulse_context = pa_context_new(pa_glib_mainloop_get_api(pa_main_loop), "ayatana.indicator.sound");
  g_assert(pulse_context);
  sink_hash = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, destroy_sink_info);
  // Establish event callback registration
  pa_context_set_state_callback(pulse_context, context_state_callback, NULL);
  dbus_menu_manager_update_pa_state(FALSE, FALSE, FALSE, 0);
  pa_context_connect(pulse_context, NULL, PA_CONTEXT_NOFAIL, NULL);
}

/**
destroy_sink_info()
item destructor method for the sink_info hash
**/
static void destroy_sink_info(void *value)
{
  sink_info *sink = (sink_info*)value;
  g_free(sink->name);
  g_free(sink);
}

/*
Controllers & Utilities
*/
static gboolean determine_sink_availability()
{
  // Firstly check to see if we have any sinks
  // if not get the hell out of here !
  if (g_hash_table_size(sink_hash) < 1) {
    g_debug("Sink_available returning false because sinks_hash is empty !!!");   
    DEFAULT_SINK_INDEX = -1;
    return FALSE;
  }
  // Secondly, make sure the default sink index is set
  // If the default sink index has not been set
  // (via the server or has been reset because default sink has been removed),
  // it will attempt to set it to the value of the first
  // index in the array of keys from the sink_hash.
  GList* keys = g_hash_table_get_keys(sink_hash);
  GList* key = g_list_first(keys);

  DEFAULT_SINK_INDEX = (DEFAULT_SINK_INDEX < 0) ? GPOINTER_TO_INT(key->data) : DEFAULT_SINK_INDEX;

  // Thirdly ensure the default sink index does not have the name "auto_null"
  sink_info* s = g_hash_table_lookup(sink_hash, GINT_TO_POINTER(DEFAULT_SINK_INDEX));
  // Up until now the most robust method to test this is to manually remove the available sink device
  // kernel module and then reload (rmmod & modprobe).
  // TODO: Edge case of dynamic loading and unloading of sinks should be handled also.
  /*    g_debug("About to test for to see if the available sink is null - s->name = %s", s->name);*/
  gboolean available = g_ascii_strncasecmp("auto_null", s->name, 9) != 0;
  /*    g_debug("PA_Manager ->  determine_sink_availability: %i", available);*/
  return available;
}

static gboolean default_sink_is_muted()
{
  if (DEFAULT_SINK_INDEX < 0)
    return FALSE;
  if (g_hash_table_size(sink_hash) < 1)
    return FALSE;
  sink_info *s = g_hash_table_lookup(sink_hash, GINT_TO_POINTER(DEFAULT_SINK_INDEX));
  return s->mute;
}

static void check_sink_input_while_muted_event(gint sink_index)
{
  /*    g_debug("SINKINPUTWHILEMUTED SIGNAL EVENT TO BE SENT FROM PA MANAGER - check trace for value");*/

  if (default_sink_is_muted(sink_index) == TRUE) {
    sound_service_dbus_sink_input_while_muted (dbus_service, TRUE);
  } else {
    sound_service_dbus_sink_input_while_muted(dbus_service, FALSE);
  }
}

static gdouble get_default_sink_volume()
{
  if (DEFAULT_SINK_INDEX < 0)
    return 0;
  sink_info *s = g_hash_table_lookup(sink_hash, GINT_TO_POINTER(DEFAULT_SINK_INDEX));
  pa_volume_t vol = pa_cvolume_avg(&s->volume);
  gdouble volume_percent = ((gdouble) vol * 100) / PA_VOLUME_NORM;
  /*    g_debug("software volume = %f", volume_percent);*/
  return volume_percent;
}

static void mute_each_sink(gpointer key, gpointer value, gpointer user_data)
{
  sink_info *info = (sink_info*)value;
  pa_operation_unref(pa_context_set_sink_mute_by_index(pulse_context, info->index, GPOINTER_TO_INT(user_data), context_success_callback,  NULL));
  if (GPOINTER_TO_INT(user_data) == 1) {
    sound_service_dbus_update_sink_mute(dbus_service, TRUE);
  } else {
		dbus_menu_manager_update_volume(get_default_sink_volume());
  }

  /*    g_debug("in the pulse manager: mute each sink %i", GPOINTER_TO_INT(user_data));*/
}

void toggle_global_mute(gboolean mute_value)
{
  g_hash_table_foreach(sink_hash, mute_each_sink, GINT_TO_POINTER(mute_value));
  /*    g_debug("in the pulse manager: toggle global mute value %i", mute_value);*/
}


/*
Refine the resolution of the slider or binary scale it to achieve a more subtle volume control.
Use the base volume stored in the sink struct to calculate actual linear volumes.
*/
void set_sink_volume(gdouble percent)
{
  if (pa_server_available == FALSE)
    return;
  /*    g_debug("in the pulse manager:set_sink_volume with percent %f", percent);*/

  if (DEFAULT_SINK_INDEX < 0) {
    g_warning("We have no default sink !!! - returning after not attempting to set any volume of any sink");
    return;
  }

  sink_info *cached_sink = g_hash_table_lookup(sink_hash, GINT_TO_POINTER(DEFAULT_SINK_INDEX));

  pa_cvolume new_volume;
  pa_cvolume_init(&new_volume);
  new_volume.channels = 1;
  pa_volume_t new_volume_value = (pa_volume_t) ((percent * PA_VOLUME_NORM) / 100);
  pa_cvolume_set(&new_volume, 1, new_volume_value);
  pa_cvolume_set(&cached_sink->volume, cached_sink->channel_map.channels, new_volume_value);
  pa_operation_unref(pa_context_set_sink_volume_by_index(pulse_context, DEFAULT_SINK_INDEX, &new_volume, NULL, NULL));
}


static pa_cvolume construct_mono_volume(const pa_cvolume* vol)
{
  pa_cvolume new_volume;
  pa_cvolume_init(&new_volume);
  new_volume.channels = 1;
  pa_volume_t max_vol = pa_cvolume_max(vol);
  pa_cvolume_set(&new_volume, 1, max_vol);
  return new_volume;
}

/**********************************************************************************************************************/
//    Pulse-Audio asychronous call-backs
/**********************************************************************************************************************/

static void gather_pulse_information(pa_context *c, void *userdata)
{
  pa_operation *operation;
  if (!(operation = pa_context_get_server_info(c, pulse_server_info_callback, userdata))) {
    g_warning("pa_context_get_server_info failed");
    if (!(operation = pa_context_get_sink_info_list(c, pulse_sink_info_callback, NULL))) {
      g_warning("pa_context_get_sink_info_list() failed - cannot fetch server or sink info - leaving . . .");
      return;
    }
  }
  pa_operation_unref(operation);
  return;
}


static void context_success_callback(pa_context *c, int success, void *userdata)
{
  /*    g_debug("Context Success Callback - result = %i", success);*/
}

/**
On Service startup this callback will be called multiple times resulting our sinks_hash container to be filled with the
available sinks.
For now this callback assumes it only used at startup. It may be necessary to use if sinks become available after startup.
Major candidate for refactoring.
**/
static void pulse_sink_info_callback(pa_context *c, const pa_sink_info *sink, int eol, void *userdata)
{
  if (eol > 0) {

    gboolean device_available = determine_sink_availability();
    if (device_available == TRUE) {
      dbus_menu_manager_update_pa_state(TRUE,
                                        device_available,
                                        default_sink_is_muted(),
                                        get_default_sink_volume());
    } else {
      //Update the indicator to show PA either is not ready or has no available sink
      g_warning("Cannot find a suitable default sink ...");
      dbus_menu_manager_update_pa_state(FALSE, device_available, default_sink_is_muted(), get_default_sink_volume());
    }
  } else {
    /*        g_debug("About to add an item to our hash");*/
    sink_info *value;
    value = g_new0(sink_info, 1);
    value->index = sink->index;
    value->name = g_strdup(sink->name);
    value->mute = !!sink->mute;
    value->volume = construct_mono_volume(&sink->volume);
    value->base_volume = sink->base_volume;
    value->channel_map = sink->channel_map;
    g_hash_table_insert(sink_hash, GINT_TO_POINTER(sink->index), value);
    /*        g_debug("After adding an item to our hash");*/
  }
}

static void pulse_default_sink_info_callback(pa_context *c, const pa_sink_info *info, int eol, void *userdata)
{
  if (eol > 0) {
    if (pa_context_errno(c) == PA_ERR_NOENTITY)
      return;
    g_warning("Default Sink info callback failure");
    return;
  } else {
    DEFAULT_SINK_INDEX = info->index;
    /*        g_debug("Just set the default sink index to %i", DEFAULT_SINK_INDEX);    */
    GList *keys = g_hash_table_get_keys(sink_hash);
    gint position =  g_list_index(keys, GINT_TO_POINTER(info->index));
    // Only update sink-list if the index is not in our already fetched list.
    if (position < 0) {
      pa_operation_unref(pa_context_get_sink_info_list(c, pulse_sink_info_callback, NULL));
    } else {
      dbus_menu_manager_update_pa_state(TRUE, determine_sink_availability(), default_sink_is_muted(), get_default_sink_volume());
    }
  }
}

static void pulse_sink_input_info_callback(pa_context *c, const pa_sink_input_info *info, int eol, void *userdata)
{
  if (eol > 0) {
    if (pa_context_errno(c) == PA_ERR_NOENTITY)
      return;
    /*        g_warning("Sink INPUT info callback failure");*/
    return;
  } else {
    if (info == NULL) {
      // TODO: watch this carefully - PA async api should not be doing this . . .
      /*            g_warning("\n Sink input info callback : SINK INPUT INFO IS NULL BUT EOL was not POSITIVE!!!");*/
      return;
    }
    /*		g_debug("\n SINK INPUT INFO sink index : %d \n", info->sink);*/
    check_sink_input_while_muted_event(info->sink);
  }
}

static void update_sink_info(pa_context *c, const pa_sink_info *info, int eol, void *userdata)
{
  if (eol > 0) {
    if (pa_context_errno(c) == PA_ERR_NOENTITY)
      return;
    /*        g_warning("Sink INPUT info callback failure");*/
    return;
  }
  gint position = -1;
  GList *keys = g_hash_table_get_keys(sink_hash);

  if (info == NULL)
    return;

  position =  g_list_index(keys, GINT_TO_POINTER(info->index));

  if (position >= 0) { // => index is within the keys of the hash.
    sink_info *s = g_hash_table_lookup(sink_hash, GINT_TO_POINTER(info->index));
    s->name = g_strdup(info->name);
    gboolean mute_changed = s->mute != !!info->mute;
    s->mute = !!info->mute;
    gboolean volume_changed = has_volume_changed(info, s);

    /*        g_debug("new balance : %i", (int)(pa_cvolume_get_balance(&info->volume, &info->channel_map) * 100));*/
    /*        g_debug("cached balance : %i", (int)(pa_cvolume_get_balance(&s->volume, &s->channel_map) * 100));*/
    /*        g_debug("update_sink_info: new_volume input : %f", (gdouble)(pa_cvolume_max(&info->volume)));*/
    /*        g_debug("update sink info: cached volume is at: %f", (gdouble)(pa_cvolume_max(&s->volume)));*/
    /*        g_debug("update sink info : volume changed = %i", volume_changed);*/
    /*        g_debug("update sink info : compatibility = %i", pa_cvolume_compatible_with_channel_map(&info->volume, &s->channel_map));*/

    s->volume = construct_mono_volume(&info->volume);
    s->channel_map = info->channel_map;

    if (DEFAULT_SINK_INDEX == s->index) {
      //update the UI
      if (volume_changed == TRUE && s->mute == FALSE) {
        pa_volume_t vol = pa_cvolume_max(&s->volume);
        gdouble volume_percent = ((gdouble) vol * 100) / PA_VOLUME_NORM;
        /*                g_debug("Updating volume from PA manager with volume = %f", volume_percent);*/
        dbus_menu_manager_update_volume(volume_percent);
      }

      if (mute_changed == TRUE) {
        /*                g_debug("Updating Mute from PA manager with mute = %i", s->mute);*/
        sound_service_dbus_update_sink_mute(dbus_service, s->mute);
        dbus_menu_manager_update_mute_ui(s->mute);
        if (s->mute == FALSE) {
          pa_volume_t vol = pa_cvolume_max(&s->volume);
          gdouble volume_percent = ((gdouble) vol * 100) / PA_VOLUME_NORM;
          /*                    g_debug("Updating volume from PA manager with volume = %f", volume_percent);*/
	        dbus_menu_manager_update_volume(volume_percent);
        }
      }
    }
  } else {
    sink_info *value;
    value = g_new0(sink_info, 1);
    value->index = info->index;
    value->name = g_strdup(info->name);
    value->mute = !!info->mute;
    value->volume = construct_mono_volume(&info->volume);
    value->channel_map = info->channel_map;
    value->base_volume = info->base_volume;
    g_hash_table_insert(sink_hash, GINT_TO_POINTER(value->index), value);
    /*        g_debug("pulse-manager:update_sink_info -> After adding a new sink to our hash");*/
    sound_service_dbus_update_sink_availability(dbus_service, TRUE);
  }
}


static gboolean has_volume_changed(const pa_sink_info* new_sink, sink_info* cached_sink)
{
  if (pa_cvolume_compatible_with_channel_map(&new_sink->volume, &cached_sink->channel_map) == FALSE)
    return FALSE;

  pa_cvolume new_vol = construct_mono_volume(&new_sink->volume);

  if (pa_cvolume_equal(&new_vol, &(cached_sink->volume)) == TRUE) {
    /*        g_debug("has_volume_changed: volumes appear to be equal? no change triggered!");    */
    return FALSE;
  }

  return TRUE;
}


static void pulse_server_info_callback(pa_context *c, const pa_server_info *info, void *userdata)
{
  /*    g_debug("server info callback");*/
  pa_operation *operation;
  if (info == NULL) {
    g_warning("No server - get the hell out of here");
    dbus_menu_manager_update_pa_state(FALSE, FALSE, TRUE, 0);
    pa_server_available = FALSE;
    return;
  }
  pa_server_available = TRUE;
  if (info->default_sink_name != NULL) {
    if (!(operation = pa_context_get_sink_info_by_name(c, info->default_sink_name, pulse_default_sink_info_callback, userdata))) {
      g_warning("pa_context_get_sink_info_by_name() failed");
    } else {
      pa_operation_unref(operation);
      return;
    }
  }
  if (!(operation = pa_context_get_sink_info_list(c, pulse_sink_info_callback, NULL))) {
    g_warning("pa_context_get_sink_info_list() failed");
    return;
  }
  pa_operation_unref(operation);
}


static void subscribed_events_callback(pa_context *c, enum pa_subscription_event_type t, uint32_t index, void *userdata)
{
  switch (t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) {
  case PA_SUBSCRIPTION_EVENT_SINK:
    if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_REMOVE) {
      if (index == DEFAULT_SINK_INDEX)
        sound_service_dbus_update_sink_availability(dbus_service, FALSE);

      /*                g_debug("Subscribed_events_callback - removing sink of index %i from our sink hash - keep the cache tidy !", index);*/
      g_hash_table_remove(sink_hash, GINT_TO_POINTER(index));

      if (index == DEFAULT_SINK_INDEX) {
        /*                    g_debug("subscribed_events_callback - PA_SUBSCRIPTION_EVENT_SINK REMOVAL: default sink %i has been removed.", DEFAULT_SINK_INDEX);  */
        DEFAULT_SINK_INDEX = -1;
        determine_sink_availability();
      }
      /*                g_debug("subscribed_events_callback - Now what is our default sink : %i", DEFAULT_SINK_INDEX);    */
    } else {
      /*			    g_debug("subscribed_events_callback - PA_SUBSCRIPTION_EVENT_SINK: a generic sink event - will trigger an update");            */
      pa_operation_unref(pa_context_get_sink_info_by_index(c, index, update_sink_info, userdata));
    }
    break;
  case PA_SUBSCRIPTION_EVENT_SINK_INPUT:
    /*			g_debug("subscribed_events_callback - PA_SUBSCRIPTION_EVENT_SINK_INPUT event triggered!!");*/
    if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_REMOVE) {
      //handle the sink input remove event - not relevant for current design
    } else {
      pa_operation_unref(pa_context_get_sink_input_info(c, index, pulse_sink_input_info_callback, userdata));
    }
    break;
  case PA_SUBSCRIPTION_EVENT_SERVER:
    g_debug("subscribed_events_callback - PA_SUBSCRIPTION_EVENT_SERVER change of some description ???");
    pa_operation *o;
    if (!(o = pa_context_get_server_info(c, pulse_server_info_callback, userdata))) {
      g_warning("subscribed_events_callback - pa_context_get_server_info() failed");
      return;
    }
    pa_operation_unref(o);
    break;
  }
}


static void context_state_callback(pa_context *c, void *userdata)
{
  switch (pa_context_get_state(c)) {
  case PA_CONTEXT_UNCONNECTED:
    /*			g_debug("unconnected");*/
    break;
  case PA_CONTEXT_CONNECTING:
    /*			g_debug("connecting - waiting for the server to become available");*/
    break;
  case PA_CONTEXT_AUTHORIZING:
    /*			g_debug("authorizing");*/
    break;
  case PA_CONTEXT_SETTING_NAME:
    /*			g_debug("context setting name");*/
    break;
  case PA_CONTEXT_FAILED:
    g_warning("FAILED to retrieve context - Is PulseAudio Daemon running ?");
    pa_server_available = FALSE;
    reconnect_to_pulse();
    break;
  case PA_CONTEXT_TERMINATED:
    /*			g_debug("context terminated");*/
    break;
  case PA_CONTEXT_READY:
    g_debug("PA daemon is ready");
    pa_operation *o;

    pa_context_set_subscribe_callback(c, subscribed_events_callback, userdata);

    if (!(o = pa_context_subscribe(c, (pa_subscription_mask_t)
                                   (PA_SUBSCRIPTION_MASK_SINK|
                                    PA_SUBSCRIPTION_MASK_SINK_INPUT|
                                    PA_SUBSCRIPTION_MASK_SERVER), NULL, NULL))) {
      g_warning("pa_context_subscribe() failed");
      return;
    }
    pa_operation_unref(o);

    gather_pulse_information(c, userdata);

    break;
  }
}

