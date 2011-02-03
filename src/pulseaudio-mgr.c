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

#define RECONNECT_DELAY 5

static void context_state_callback(pa_context *c, void *userdata);

static gint reconnect_idle_id = 0;
static pa_context *pulse_context = NULL;
static pa_glib_mainloop *pa_main_loop = NULL;

// Entry Point
void establish_pulse_activities()
{
  pa_main_loop = pa_glib_mainloop_new(g_main_context_default());
  g_assert(pa_main_loop);
  pulse_context = pa_context_new(pa_glib_mainloop_get_api(pa_main_loop),
                                 "com.canonical.indicators.sound");
  g_assert(pulse_context);

  pa_context_set_state_callback (pulse_context, context_state_callback, /*active sink obj*/NULL);
  sound_service_dbus_update_pa_state (dbus_service, FALSE, FALSE, 0);
  pa_context_connect (pulse_context, NULL, PA_CONTEXT_NOFAIL, /*active sink obj*/NULL);  
}

static gboolean
reconnect_to_pulse()
{
  g_debug("Attempt to reconnect to pulse");
  // reset
  if (pulse_context != NULL) {
    pa_context_unref(pulse_context);
    pulse_context = NULL;
  }

  pulse_context = pa_context_new( pa_glib_mainloop_get_api( pa_main_loop ),
                                  "com.canonical.indicators.sound" );
  g_assert(pulse_context);
  pa_context_set_state_callback (pulse_context, context_state_callback, NULL);
  int result = pa_context_connect (pulse_context, NULL, PA_CONTEXT_NOFAIL, NULL);

  if (result < 0) {
    g_warning ("Failed to connect context: %s",
               pa_strerror (pa_context_errno (pulse_context)));
  }
  // we always want to cancel any continious callbacks with the existing timeout
  // if the connection failed the new context created above will catch any updates
  // to do with the state of pulse and thus take care of business.
  reconnect_idle_id = 0;  
  return FALSE;
}


static void pulse_server_info_callback(pa_context *c,
                                       const pa_server_info *info,
                                       void *userdata)
{
  pa_operation *operation;
  if (info == NULL) {
    g_warning("No server - get the hell out of here");
    //sound_service_dbus_update_pa_state(dbus_service, FALSE, TRUE, 0);
    return;
  }
  if (info->default_sink_name != NULL) {
    if (!(operation = pa_context_get_sink_info_by_name(c,
                                                       info->default_sink_name,
                                                       pulse_default_sink_info_callback,
                                                       userdata))) {
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
        sound_service_dbus_update_sound_state(dbus_service, UNAVAILABLE);

      /*                g_debug("Subscribed_events_callback - removing sink of index %i from our sink hash - keep the cache tidy !", index);*/
      g_hash_table_remove(sink_hash, GINT_TO_POINTER(index));

      if (index == DEFAULT_SINK_INDEX) {
        /*                    g_debug("subscribed_events_callback - PA_SUBSCRIPTION_EVENT_SINK REMOVAL: default sink %i has been removed.", DEFAULT_SINK_INDEX);  */
        DEFAULT_SINK_INDEX = -1;
        determine_sink_availability();
      }
      /*                g_debug("subscribed_events_callback - Now what is our default sink : %i", DEFAULT_SINK_INDEX);    */
    } else {
      /*          g_debug("subscribed_events_callback - PA_SUBSCRIPTION_EVENT_SINK: a generic sink event - will trigger an update");            */
      pa_operation_unref(pa_context_get_sink_info_by_index(c, index, update_sink_info, userdata));
    }
    break;
  case PA_SUBSCRIPTION_EVENT_SINK_INPUT:
    /*      g_debug("subscribed_events_callback - PA_SUBSCRIPTION_EVENT_SINK_INPUT event triggered!!");*/
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


static void
context_state_callback (pa_context *c, void *userdata)
{
  switch (pa_context_get_state(c)) {
  case PA_CONTEXT_UNCONNECTED:
          g_debug("unconnected");
    break;
  case PA_CONTEXT_CONNECTING:
          g_debug("connecting - waiting for the server to become available");
    break;
  case PA_CONTEXT_AUTHORIZING:
    /*      g_debug("authorizing");*/
    break;
  case PA_CONTEXT_SETTING_NAME:
    /*      g_debug("context setting name");*/
    break;
  case PA_CONTEXT_FAILED:
    g_warning("PA_CONTEXT_FAILED - Is PulseAudio Daemon running ?");
    /*sound_service_dbus_update_pa_state( dbus_service,
                                        pa_server_available,
                                        default_sink_is_muted(),
                                        get_default_sink_volume() );
    */  
    if (reconnect_idle_id == 0){
      reconnect_idle_id = g_timeout_add_seconds (RECONNECT_DELAY,
                                                 reconnect_to_pulse,
                                                 NULL);                                                      
    }     
    break;
  case PA_CONTEXT_TERMINATED:
    /*      g_debug("context terminated");*/
    break;
  case PA_CONTEXT_READY:
    g_debug("PA_CONTEXT_READY");
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

    //gather_pulse_information(c, userdata);

    break;
  }
}
