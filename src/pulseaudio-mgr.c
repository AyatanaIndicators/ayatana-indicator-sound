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
 * Approach now is to set up the communication channels, query the server
 * fetch its default sink/source. If this fails then fetch the list of sinks/sources
 * and take the first one which is not the auto-null sink.
 * TODO: need to handle the situation where one chink in this linear chain breaks
 * i.e. start off the process again and count the attempts (note different to
                                                            reconnect attempts)
 */
#include <pulse/gccmacro.h>
#include <pulse/glib-mainloop.h>
#include <pulse/error.h>

#include "pulseaudio-mgr.h"
#include "config.h"

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
static void pm_default_source_info_callback (pa_context *c,
                                             const pa_source_info *info,
                                             int eol,
                                             void *userdata);
static void pm_sink_info_callback (pa_context *c,
                                   const pa_sink_info *sink,
                                   int eol,
                                   void *userdata);
static void pm_source_info_callback (pa_context *c,
                                     const pa_source_info *info,
                                     int eol,
                                     void *userdata);
static void pm_update_source_info_callback (pa_context *c,
                                            const pa_source_info *info,
                                            int eol,
                                            void *userdata);
static void pm_sink_input_info_callback (pa_context *c,
                                         const pa_sink_input_info *info,
                                         int eol,
                                         void *userdata);
static void pm_update_device (pa_context *c,
                                   const pa_sink_info *info,
                                   int eol,
                                   void *userdata);
static void pm_toggle_mute_for_every_sink_callback (pa_context *c,
                                                    const pa_sink_info *sink,
                                                    int eol,
                                                    void* userdata);
static void pm_source_output_info_callback (pa_context *c,
                                            const pa_source_output_info *info,
                                            int eol,
                                            void *userdata);

static gboolean reconnect_to_pulse (gpointer user_data);

static gint connection_attempts = 0;
static gint reconnect_idle_id = 0;
static pa_context *pulse_context = NULL;
static pa_glib_mainloop *pa_main_loop = NULL;

/**
 Entry Point
 **/
void 
pm_establish_pulse_connection (Device* device)
{
  pa_main_loop = pa_glib_mainloop_new (g_main_context_default ());
  g_assert (pa_main_loop);
  reconnect_to_pulse ((gpointer)device);
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

/**
reconnect_to_pulse (gpointer user_data)
Method which connects to the pulse server and is used to track reconnects.
 */
static gboolean
reconnect_to_pulse (gpointer user_data)
{
  g_debug("Attempt a pulse connection");
  g_return_val_if_fail (IS_DEVICE (user_data), FALSE);

  connection_attempts += 1;
  if (pulse_context != NULL) {
    pa_context_unref(pulse_context);
    pulse_context = NULL;
  }

  pa_proplist     *proplist;

  proplist = pa_proplist_new ();
  pa_proplist_sets (proplist,
                    PA_PROP_APPLICATION_NAME,
                    "Indicator Sound");
  pa_proplist_sets (proplist,
                    PA_PROP_APPLICATION_ID,
                    "com.canonical.indicator.sound");
  pa_proplist_sets (proplist,
                    PA_PROP_APPLICATION_ICON_NAME,
                    "multimedia-volume-control");
  pa_proplist_sets (proplist,
                    PA_PROP_APPLICATION_VERSION,
                    PACKAGE_VERSION);

  pulse_context = pa_context_new_with_proplist (pa_glib_mainloop_get_api( pa_main_loop ),
                                                NULL,
                                                proplist);
  pa_proplist_free (proplist);
  g_assert(pulse_context);
  pa_context_set_state_callback (pulse_context,
                                 pm_context_state_callback,
                                 user_data);
  int result = pa_context_connect (pulse_context,
                                   NULL,
                                   (pa_context_flags_t)PA_CONTEXT_NOFAIL,
                                   NULL);

  if (result < 0) {
    g_warning ("Failed to connect context: %s",
               pa_strerror (pa_context_errno (pulse_context)));
  }
  if (connection_attempts > 5){
    return FALSE;
  }
  else{
    return TRUE;
  }
}

void
pm_update_volume (gint sink_index, pa_cvolume new_volume)
{
  if (sink_index < 0 || pulse_context == NULL){
    g_warning ("pm_update_volume sink index is negative or the context is null");    
    return;
  }
  
  if (pa_context_get_state (pulse_context) != PA_CONTEXT_READY ){
    g_warning ("pm_update_volume context is not in a ready state");    
    return;    
  }

  pa_operation *operation = NULL;
  
  operation = pa_context_set_sink_volume_by_index (pulse_context,
                                                   sink_index,
                                                   &new_volume,
                                                   NULL,
                                                   NULL);
  if (!operation){
    g_warning ("pm_update_volume operation failed for some reason");
    return;
  }                                                    
  pa_operation_unref (operation);
}

void
pm_update_mute (gboolean update)
{
  if (pulse_context == NULL){
    g_warning ("pm_update_mute - the context is null");    
    return;
  }

  if (pa_context_get_state (pulse_context) != PA_CONTEXT_READY ){
    g_warning ("pm_update_mute context is not in a ready state");    
    return;    
  }

  pa_operation *operation = NULL;
  
  operation =  pa_context_get_sink_info_list (pulse_context,
                                              pm_toggle_mute_for_every_sink_callback,
                                              GINT_TO_POINTER (update));
  if (!operation){
    g_warning ("pm_update_mute operation failed for some reason");
    return;
  }
  pa_operation_unref (operation);
}

void
pm_update_mic_gain (gint source_index, pa_cvolume new_gain)
{
  if (source_index < 0 || pulse_context == NULL){
    g_warning ("pm_update_mic_gain source index is negative or the context is null");        
    return;
  }  

  if (pa_context_get_state (pulse_context) != PA_CONTEXT_READY ){
    g_warning ("pm_update_mic_gain context is not in a ready state");    
    return;    
  }
  
  pa_operation *operation = NULL;
  
  operation = pa_context_set_source_volume_by_index (pulse_context,
                                                     source_index,
                                                     &new_gain,
                                                     NULL,
                                                     NULL);
  if (!operation){
    g_warning ("pm_update_mic_gain operation failed for some reason");
    return;
  }
  pa_operation_unref (operation);                                                     
}

void
pm_update_mic_mute (gint source_index, gint mute_update)
{
  if (source_index < 0){
    return;
  }

  if (pa_context_get_state (pulse_context) != PA_CONTEXT_READY ){
    g_warning ("pm_update_mic_mute context is not in a ready state");    
    return;    
  }

  pa_operation *operation = NULL;

  operation = pa_context_set_source_mute_by_index (pulse_context,
                                                   source_index,
                                                   mute_update,
                                                   NULL,
                                                   NULL);
  if (!operation){
    g_warning ("pm_update_mic_mute operation failed for some reason");
    return;
  }
  pa_operation_unref (operation);                                                   
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
  if (IS_DEVICE (userdata) == FALSE){
    g_critical ("subscribed events callback - our userdata is not what we think it should be");
    return;
  }
  Device* sink = DEVICE (userdata);

  switch (t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) {
  case PA_SUBSCRIPTION_EVENT_SINK:
    
    // We don't care about any other sink other than the active one.
    if (index != device_get_sink_index (sink))
      return;
      
    if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_REMOVE) {
      device_sink_deactivated (sink);
      
    }
    else{
      pa_operation_unref (pa_context_get_sink_info_by_index (c,
                                                             index,
                                                             pm_update_device,
                                                             userdata) );
    }
    break;
  case PA_SUBSCRIPTION_EVENT_SOURCE:
    g_debug ("Looks like source event of some description - index = %i", index);
    // We don't care about any other sink other than the active one.
    if (index != device_get_source_index (sink))
        return;
    if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_REMOVE) {
      g_debug ("Source removal event - index = %i", index);
      device_deactivate_voip_source (sink, FALSE);
    }
    else{
      pa_operation_unref (pa_context_get_source_info_by_index (c,
                                                               index,
                                                               pm_update_source_info_callback,
                                                               userdata) );
    }
    break;
  case PA_SUBSCRIPTION_EVENT_SINK_INPUT:
    if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_NEW) {
      g_debug ("some new sink input event ? - index = %i", index);
      // Maybe blocking state ?.
      pa_operation_unref (pa_context_get_sink_input_info (c,
                                                          index,
                                                          pm_sink_input_info_callback, userdata));
    }
    break;
  case PA_SUBSCRIPTION_EVENT_SOURCE_OUTPUT:
    g_debug ("source output event");
    if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_REMOVE) {
      gint cached_source_output_index = device_get_voip_source_output_index (sink);
      if (index == cached_source_output_index){
        g_debug ("Just saw a source output removal event - index = %i and cached index = %i", index, cached_source_output_index);
        device_deactivate_voip_client (sink);
      }
    }
    else if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_NEW) {
      g_debug ("some new source output event ? - index = %i", index);
      // Determine if its a VOIP app.
      pa_operation_unref (pa_context_get_source_output_info (c,
                                                            index,
                                                            pm_source_output_info_callback, userdata));
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
    g_debug ("Authorizing");
    break;
  case PA_CONTEXT_SETTING_NAME:
    g_debug ("Setting name");
    break;
  case PA_CONTEXT_FAILED:
    g_warning("PA_CONTEXT_FAILED - Is PulseAudio Daemon running ?");
    device_sink_deactivated (DEVICE (userdata));
    if (reconnect_idle_id == 0){
      reconnect_idle_id = g_timeout_add_seconds (RECONNECT_DELAY,
                                                 reconnect_to_pulse,
                                                 (gpointer)userdata);
    }
    break;
  case PA_CONTEXT_TERMINATED:
    g_debug ("Terminated");
    device_sink_deactivated (DEVICE (userdata));

    if (reconnect_idle_id != 0){
      g_source_remove (reconnect_idle_id);
      reconnect_idle_id = 0;
    }
    break;
  case PA_CONTEXT_READY:
    connection_attempts = 0;
    g_debug("PA_CONTEXT_READY");
    
    if (reconnect_idle_id != 0){
      g_source_remove (reconnect_idle_id);
      reconnect_idle_id = 0;
    }

    pa_context_set_subscribe_callback(c, pm_subscribed_events_callback, userdata);
    pa_operation *o = NULL;

    o = pa_context_subscribe (c, (pa_subscription_mask_t)
                                 (PA_SUBSCRIPTION_MASK_SINK|
                                  PA_SUBSCRIPTION_MASK_SOURCE|
                                  PA_SUBSCRIPTION_MASK_SINK_INPUT|
                                  PA_SUBSCRIPTION_MASK_SOURCE_OUTPUT|
                                  PA_SUBSCRIPTION_MASK_SERVER),
                                  NULL,
                                  NULL);
                                   
    
    if (!o){
      g_critical("pa_context_subscribe() failed - ?");
      return;
    }

    pa_operation_unref(o);

    o = pa_context_get_server_info (c, pm_server_info_callback, userdata);

    if (!o){
      g_warning("pa_context_get_server_info() failed - ?");
      return;
    }
    
    pa_operation_unref(o);
      
    break;
  }
}

/**
 After startup we go straight for the server info to see if it has details of
 the default sink and source. Normally these are valid, if there is none set
 fetch the list of each and try to determine the sink.
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
    device_sink_deactivated (DEVICE (userdata));
    return;
  }
  // Go for the default sink
  if (info->default_sink_name != NULL) {
    g_debug ("default sink name from the server ain't null'");
    if (!(operation = pa_context_get_sink_info_by_name (c,
                                                       info->default_sink_name,
                                                       pm_default_sink_info_callback,
                                                       userdata) )) {
      g_warning("pa_context_get_sink_info_by_namet() failed");
      device_sink_deactivated (DEVICE (userdata));
      pa_operation_unref(operation);
      return;
    }
  } // If there is no default sink, try to determine a sink from the list of sinks
  else if (!(operation = pa_context_get_sink_info_list(c,
                                                       pm_sink_info_callback,
                                                       userdata))) {
    g_warning("pa_context_get_sink_info_list() failed");
    device_sink_deactivated (DEVICE (userdata));
    pa_operation_unref(operation);
    return;
  }
  // And the source
  if (info->default_source_name != NULL) {
    g_debug ("default source name from the server is not null'");
    if (!(operation = pa_context_get_source_info_by_name (c,
                                                          info->default_source_name,
                                                          pm_default_source_info_callback,
                                                          userdata) )) {
      g_warning("pa_context_get_default_source_info() failed");
      //  TODO: call some input deactivate method on active sink
      pa_operation_unref(operation);
      return;
    }
  }
  else if (!(operation = pa_context_get_source_info_list(c,
                                                         pm_source_info_callback,
                                                         userdata))) {
    g_warning("pa_context_get_sink_info_list() failed");
    //  TODO: call some input deactivate method for the source
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
    if (IS_DEVICE (userdata) == FALSE || sink == NULL){
      g_warning ("sink info callback - our user data is not what we think it should be or the sink parameter is null");
      return;
    }
    Device* a_sink = DEVICE (userdata);
    if (device_is_sink_populated (a_sink) == FALSE &&
        g_ascii_strncasecmp("auto_null", sink->name, 9) != 0){
      device_sink_populate (a_sink, sink);
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
    return;
  } 
  else {
    if (IS_DEVICE (userdata) == FALSE || info == NULL){
      g_warning ("Default sink info callback - our user data is not what we think it should be or the info parameter is null");
      return;
    }
    // Only repopulate if there is a change with regards the index
    if (device_get_sink_index (DEVICE (userdata)) == info->index)
      return;
    
    g_debug ("Pulse Server has handed us a new default sink");
    device_sink_populate (DEVICE (userdata), info);
  }
}

static void 
pm_sink_input_info_callback (pa_context *c,
                             const pa_sink_input_info *info,
                             int eol,
                             void *userdata)
{
  if (eol > 0) {
    return;
  }
  else {
    if (info == NULL || IS_DEVICE (userdata) == FALSE) {
      g_warning("Sink input info callback : SINK INPUT INFO IS NULL or our user_data is not what we think it should be");
      return;
    }
    Device* a_sink = DEVICE (userdata);
    // And finally check for the mute blocking state
    if (device_get_sink_index (a_sink) == info->sink){
      device_determine_blocking_state (a_sink);
    }
  }
}

static void
pm_source_output_info_callback (pa_context *c,
                                const pa_source_output_info *info,
                                int eol,
                                void *userdata)
{
  if (eol > 0) {
    return;
  }
  else {
    if (info == NULL || IS_DEVICE (userdata) == FALSE) {
      g_warning("Source output callback: SOURCE OUTPUT INFO IS NULL or our user_data is not what we think it should be");
      return;
    }

    // Check if this is Voip sink input
    gint result  = pa_proplist_contains (info->proplist, PA_PROP_MEDIA_ROLE);
    Device* a_sink = DEVICE (userdata);

    if (result == 1){
      //g_debug ("Source output info has media role property");
      const char* value = pa_proplist_gets (info->proplist, PA_PROP_MEDIA_ROLE);
      //g_debug ("prop role = %s", value);
      if (g_strcmp0 (value, "phone") == 0 || g_strcmp0 (value, "production") == 0) {
        g_debug ("We have a VOIP/PRODUCTION ! - index = %i", info->index);
        device_activate_voip_item (a_sink, (gint)info->index, (gint)info->client);
        // TODO to start with we will assume our source is the same as what this 'client'
        // is pointing at. This should probably be more intelligent :
        // query for the list of source output info's and going on the name of the client
        // from the sink input ensure our voip item is using the right source.
      }
    }
  }
}

static void 
pm_update_device (pa_context *c,
                       const pa_sink_info *info,
                       int eol,
                       void *userdata)
{
  if (eol > 0) {
    return;
  }
  else{
    if (IS_DEVICE (userdata) == FALSE || info == NULL){
      g_warning ("update_device - our user data is not what we think it should be or the info parameter is null");
      return;
    }
    device_sink_update (DEVICE(userdata), info);
  }
}

static void
pm_toggle_mute_for_every_sink_callback (pa_context *c,
                                        const pa_sink_info *sink,
                                        int eol,
                                        void* userdata)
{
  if (eol > 0) {
    return;
  }

  if (sink == NULL) {
    g_warning ("toggle_mute cb - sink parameter is null - why ?");
    return;
  }

  pa_operation *operation = NULL;
  operation =  pa_context_set_sink_mute_by_index (c,
                                                  sink->index,
                                                  GPOINTER_TO_INT(userdata),
                                                  NULL,
                                                  NULL);
  if (!operation){
    g_warning ("pm_update_mic_mute operation failed for some reason");
    return;
  }
  pa_operation_unref (operation);
}

// Source info related callbacks
static void
pm_default_source_info_callback (pa_context *c,
                                 const pa_source_info *info,
                                 int eol,
                                 void *userdata)
{
  if (eol > 0) {
    return;
  }
  else {
    if (IS_DEVICE (userdata) == FALSE || info == NULL){
      g_warning ("Default source info callback - our user data is not what we think it should be or the source info parameter is null");
      return;
    }
    // If there is an index change we need to change our cached source
    if (device_get_source_index (DEVICE (userdata)) == info->index)
      return;
    g_debug ("Pulse Server has handed us a new default source");
    device_deactivate_voip_source (DEVICE (userdata), TRUE);
    device_update_voip_input_source (DEVICE (userdata), info);
  }
}

static void
pm_source_info_callback (pa_context *c,
                         const pa_source_info *info,
                         int eol,
                         void *userdata)
{
  if (eol > 0) {
    return;
  }
  else {
    if (IS_DEVICE (userdata) == FALSE || info == NULL){
      g_warning ("source info callback - our user data is not what we think it should be or the source info parameter is null");
      return;
    }
    // For now we will take the first available
    if (device_is_voip_source_populated (DEVICE (userdata)) == FALSE){
      device_update_voip_input_source (DEVICE (userdata), info);
    }
  }
}

static void
pm_update_source_info_callback (pa_context *c,
                                const pa_source_info *info,
                                int eol,
                                void *userdata)
{
  if (eol > 0) {
    return;
  }
  else {
    if (IS_DEVICE (userdata) == FALSE || info == NULL ){
      g_warning ("source info update callback - our user data is not what we think it should be or the source info paramter is null");
      return;
    }
    g_debug ("Got a source update for %s , index %i", info->name, info->index);
    device_update_voip_input_source (DEVICE (userdata), info);
  }
}
