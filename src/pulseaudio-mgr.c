/*
Copyright 2011 Canonical Ltd.

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


#include <pulse/gccmacro.h>

#include "pulse-manager.h"
#include "active-sink.h"

#define RECONNECT_DELAY 5


static void pm_context_state_callback(pa_context *c, void *userdata);
static void pm_subscribed_events_callback (pa_context *c,
                                           enum pa_subscription_event_type t,
                                           uint32_t index,
                                           void* userdata);
static void pm_server_info_callback (pa_context *c,
                                     const pa_server_info *info,
                                     void *userdata);


static gint connection_attempts = 0;
static gint reconnect_idle_id = 0;
static pa_context *pulse_context = NULL;
static pa_glib_mainloop *pa_main_loop = NULL;

// Entry Point
void 
establish_pulse_activities (ActiveSink* active_sink)
{
  pa_main_loop = pa_glib_mainloop_new (g_main_context_default ());
  g_assert (pa_main_loop);
  pulse_context = pa_context_new (pa_glib_mainloop_get_api (pa_main_loop),
                                 "com.canonical.indicators.sound");
  g_assert (pulse_context);

  pa_context_set_state_callback (pulse_context,
                                 pm_context_state_callback,
                                 (gpointer)active_sink);
  
  //TODO update active sink before init with state at unavailable
  
  pa_context_connect (pulse_context, NULL, PA_CONTEXT_NOFAIL, (gpointer)active_sink);  
}

static gboolean
reconnect_to_pulse (gpointer user_data)
{
  g_debug("Attempt to reconnect to pulse");
  // reset
  connection_attempts += 1;
  if (pulse_context != NULL) {
    pa_context_unref(pulse_context);
    pulse_context = NULL;
  }

  pulse_context = pa_context_new( pa_glib_mainloop_get_api( pa_main_loop ),
                                  "com.canonical.indicators.sound" );
  g_assert(pulse_context);
  pa_context_set_state_callback (pulse_context,
                                 pm_context_state_callback,
                                 user_data);
  int result = pa_context_connect (pulse_context,
                                   NULL,
                                   PA_CONTEXT_NOFAIL,
                                   user_data);

  if (result < 0) {
    g_warning ("Failed to connect context: %s",
               pa_strerror (pa_context_errno (pulse_context)));
  }
  
  reconnect_idle_id = 0;  
  if (connection_attempts > 5){
    return FALSE;
  }
  else{
    return TRUE;
  }
}

static void 
populate_active_sink (const pa_sink_info *info, ActiveSink* sink)
{
  
}

/**********************************************************************************************************************/
//    Pulse-Audio asychronous call-backs
/**********************************************************************************************************************/


static void 
pm_subscribed_events_callback (pa_context *c,
                               enum pa_subscription_event_type t,
                               uint32_t index,
                               void* userdata)
{
  switch (t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) {
  case PA_SUBSCRIPTION_EVENT_SINK:
    if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_REMOVE) {
      // TODO check the sink index is not your active index and react appropriately
    }
    break;
  case PA_SUBSCRIPTION_EVENT_SINK_INPUT:
    if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_REMOVE) {
      //handle the sink input remove event - not relevant for current design
    } 
    else {
      // query the info of the sink input to see if we have a blocking moment
      // TODO investigate what the id is here.
      pa_operation_unref (pa_context_get_sink_input_info (c,
                                                          index,
                                                          pulse_sink_input_info_callback, userdata));
    }
    break;
  case PA_SUBSCRIPTION_EVENT_SERVER:
    g_debug("PA_SUBSCRIPTION_EVENT_SERVER event triggered.");
    pa_operation *o;
    if (!(o = pa_context_get_server_info (c, pulse_server_info_callback, userdata))) {
      g_warning("subscribed_events_callback - pa_context_get_server_info() failed");
      return;
    }
    pa_operation_unref(o);
    break;
  }
}


static void
pm_context_state_callback (pa_context *c, void *userdata)
{
  switch (pa_context_get_state(c)) {
  case PA_CONTEXT_UNCONNECTED:
    g_debug("unconnected");
    break;
  case PA_CONTEXT_CONNECTING:
    g_debug("connecting - waiting for the server to become available");
    break;
  case PA_CONTEXT_AUTHORIZING:
    break;
  case PA_CONTEXT_SETTING_NAME:
    break;
  case PA_CONTEXT_FAILED:
    g_warning("PA_CONTEXT_FAILED - Is PulseAudio Daemon running ?");
    // TODO: update state to unvailable on active sink
    if (reconnect_idle_id == 0){
      reconnect_idle_id = g_timeout_add_seconds (RECONNECT_DELAY,
                                                 reconnect_to_pulse,
                                                 userdata);                                                      
    }     
    break;
  case PA_CONTEXT_TERMINATED:
    break;
  case PA_CONTEXT_READY:
          
    connection_attempts = 0;
    g_debug("PA_CONTEXT_READY");
    pa_operation *o;
            
    pa_context_set_subscribe_callback(c, pm_subscribed_events_callback, userdata);

    if (!(o = pa_context_subscribe (c, (pa_subscription_mask_t)
                                   (PA_SUBSCRIPTION_MASK_SINK|
                                    PA_SUBSCRIPTION_MASK_SINK_INPUT|
                                    PA_SUBSCRIPTION_MASK_SERVER), NULL, NULL))) {
      g_warning("pa_context_subscribe() failed");
      return;
    }
    pa_operation_unref(o);

    //gather_pulse_information(c, userdata);

    break;
  }
}

/**
 After startup we go straight for the server info to see if it has details of
 the default sink. If so it makes things much easier.
 **/
static void 
pm_server_info_callback (pa_context *c,
                         const pa_server_info *info,
                         void *userdata)
{
  pa_operation *operation;
  if (info == NULL) {
    g_warning("No PA server - get the hell out of here");
    //TODO update active sink with state info
    return;
  }
  if (info->default_sink_name != NULL) {
    if (!(operation = pa_context_get_sink_info_by_name (c,
                                                       info->default_sink_name,
                                                       pm_default_sink_info_callback,
                                                       userdata) )) {
    } 
    else{
      pa_operation_unref(operation);
      return;
    }
  }
  else if (!(operation = pa_context_get_sink_info_list(c, pulse_sink_info_callback, NULL))) {
    g_warning("pa_context_get_sink_info_list() failed");
    return;
  }
  pa_operation_unref(operation);
}

static void
pm_sink_info_callback (pa_context *c,
                       const pa_sink_info *sink,
                       int eol,
                       void *userdata)
{
  if (eol > 0) {
    return;
  }
  else {
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

static void
pm_default_sink_info_callback (pa_context *c,
                               const pa_sink_info *info,
                               int eol,
                               void *userdata)
{
  if (eol > 0) {
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
      sound_service_dbus_update_pa_state(dbus_service,
                                        determine_sink_availability(),
                                        default_sink_is_muted(),
                                        get_default_sink_volume());
    }
  }
}


