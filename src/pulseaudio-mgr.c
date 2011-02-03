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

/**Notes
 *
 * Approach now is to set up the communication channels then query the server
 * fetch its default sink. If this fails then fetch the list of sinks and take
 * the first one which is not the auto-null sink.
 * TODO: need to handle the situation where one chink in this linear chain breaks
 * i.e. start off the process again and count the attempts (note different to
                                                            reconnect attempts)
 */
#include <pulse/gccmacro.h>
#include <pulse/glib-mainloop.h>
#include <pulse/error.h>

#include "pulse-manager.h"

#define RECONNECT_DELAY 5


static void pm_context_state_callback(pa_context *c, void *userdata);
static void pm_subscribed_events_callback (pa_context *c,
                                           enum pa_subscription_event_type t,
                                           uint32_t index,
                                           void* userdata);
static void pm_server_info_callback (pa_context *c,
                                     const pa_server_info *info,
                                     void *userdata);
static void pm_default_sink_info_callback (pa_context *c,
                                           const pa_sink_info *info,
                                           int eol,
                                           void *userdata);
static void pm_sink_info_callback (pa_context *c,
                                   const pa_sink_info *sink,
                                   int eol,
                                   void *userdata);


static void populate_active_sink (const pa_sink_info *info, ActiveSink* sink);
static gboolean reconnect_to_pulse (gpointer user_data);
static pa_cvolume construct_mono_volume(const pa_cvolume* vol);


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

/**
close_pulse_activites()
Gracefully close our connection with the Pulse async library.
**/
void close_pulse_activites()
{
  if (pulse_context != NULL) {
    pa_context_unref(pulse_context);
    pulse_context = NULL;
  }
  pa_glib_mainloop_free(pa_main_loop);
  pa_main_loop = NULL;
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
  sink_details *details;
  details = g_new0 (sink_details, 1);
  details->index = info->index;
  details->name = g_strdup (info->name);
  details->mute = !!info->mute;
  details->volume = construct_mono_volume (&info->volume);
  details->base_volume = info->base_volume;
  details->channel_map = info->channel_map;
  active_sink_update_details (sink, details);
  g_debug ("active sink populated with sink %s", details->name);
}

static pa_cvolume
construct_mono_volume(const pa_cvolume* vol)
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
      //pa_operation_unref (pa_context_get_sink_input_info (c,
      //                                                    index,
      //                                                    pulse_sink_input_info_callback, userdata));
    }
    break;
  case PA_SUBSCRIPTION_EVENT_SERVER:
    g_debug("PA_SUBSCRIPTION_EVENT_SERVER event triggered.");
    pa_operation *o;
    if (!(o = pa_context_get_server_info (c, pm_server_info_callback, userdata))) {
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
    
    }

    if (!(o = pa_context_get_server_info (c, pm_server_info_callback, userdata))) {
      g_warning("Initial - pa_context_get_server_info() failed");
    }
    pa_operation_unref(o);

  

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
  g_debug ("server info callback");

  if (info == NULL) {
    g_warning("No PA server - get the hell out of here");
    //TODO update active sink with state info
    return;
  }
  if (info->default_sink_name != NULL) {
    g_debug ("default sink name from the server ain't null'");
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
  else if (!(operation = pa_context_get_sink_info_list(c,
                                                       pm_sink_info_callback,
                                                       NULL))) {
    g_warning("pa_context_get_sink_info_list() failed");
    return;
  }
  pa_operation_unref(operation);
}

// If the server doesn't have a default sink to give us
// we should attempt to pick up the first of the list of sinks which doesn't have
// the name 'auto_null' (that was all really I was doing before)
static void
pm_sink_info_callback (pa_context *c,
                       const pa_sink_info *sink,
                       int eol,
                       void* userdata)
{
  if (eol > 0) {
    return;
  }
  else {
    if (IS_ACTIVE_SINK (userdata) == FALSE){
      g_warning ("sink info callback - our user data is not what we think it should be");
      return;
    }
    ActiveSink* a_sink = ACTIVE_SINK (userdata);
    if (active_sink_is_populated (a_sink) &&
        g_ascii_strncasecmp("auto_null", sink->name, 9) != 0){
      populate_active_sink (sink, a_sink);         
    }
  }
}

static void
pm_default_sink_info_callback (pa_context *c,
                               const pa_sink_info *info,
                               int eol,
                               void *userdata)
{
  if (eol > 0) {
    // TODO what happens here - high and dry!
    return;
  } 
  else {
    g_debug ("server has handed us a default sink");
    populate_active_sink (info, ACTIVE_SINK (userdata));
  }
}


