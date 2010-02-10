#include <pulse/glib-mainloop.h>
#include <pulse/error.h>
#include <pulse/gccmacro.h>

#include "pulse-manager.h"
#include "sound-service.h"


static GHashTable *sink_hash = NULL;
static SoundServiceDbus *dbus_service = NULL;
// Until we find a satisfactory default sink this index should remain < 0
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


/*
Entry point
*/
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
	pa_context_connect(pulse_context, NULL, PA_CONTEXT_NOAUTOSPAWN, NULL);    
}

void close_pulse_activites()
{
    if (pulse_context){
 	    pa_context_unref(pulse_context);
        pulse_context = NULL;
   	}
    g_hash_table_destroy(sink_hash);
    pa_glib_mainloop_free(pa_main_loop);
    pa_main_loop = NULL;
    g_debug("I just closed communication with Pulse");
}


static void destroy_sink_info(void *value)
{
    sink_info *sink = (sink_info*)value;
    g_free(sink->name);
    g_free(sink->description);        
    g_free(sink->icon_name);  
    g_free(sink);  
}

/*static void test_hash(){*/
/*    guint size = 0;*/
/*    size = g_hash_table_size(sink_hash);*/
/*    g_debug("Size of hash = %i", size);*/
/*    sink_info *s = g_hash_table_lookup(sink_hash, GINT_TO_POINTER(DEFAULT_SINK_INDEX));   */
/*    g_debug("The name of our sink is %s", s->name); */
/*    g_debug("and the max volume is %f", (gdouble)s->base_volume); */

/*}*/

/*
Controllers & Utilities
*/

static gboolean sink_available()
{
    if (g_hash_table_size(sink_hash) < 1)
        return FALSE;
    sink_info *s = g_hash_table_lookup(sink_hash, GINT_TO_POINTER(DEFAULT_SINK_INDEX));   
    // TODO more testing is required for the case of having no available sink
    // This will need to iterate through the sinks to find an available
    // one as opposed to just picking the first
    return ((g_strcasecmp(s->name, " auto_null ") != 0) && s->active_port == TRUE);
}

static gboolean default_sink_is_muted()
{
    if(DEFAULT_SINK_INDEX < 0)
        return FALSE;
    if (g_hash_table_size(sink_hash) < 1)
        return FALSE;
    sink_info *s = g_hash_table_lookup(sink_hash, GINT_TO_POINTER(DEFAULT_SINK_INDEX));   
    return s->mute;
}

static void check_sink_input_while_muted_event(gint sink_index)
{
    g_debug("SINKINPUTWHILEMUTED SIGNAL EVENT TO BE SENT FROM PA MANAGER - check trace for value");

    if (default_sink_is_muted(sink_index) == TRUE)
    {
        sound_service_dbus_sink_input_while_muted (dbus_service, TRUE);
    }
    else
    {
        sound_service_dbus_sink_input_while_muted(dbus_service, FALSE);
    }
}

static gdouble get_default_sink_volume()
{
    if (DEFAULT_SINK_INDEX < 0)
        return 0;
    sink_info *s = g_hash_table_lookup(sink_hash, GINT_TO_POINTER(DEFAULT_SINK_INDEX));
    pa_volume_t vol = pa_cvolume_avg(&s->volume);
    gdouble value = pa_sw_volume_to_linear(vol);
    g_debug("software volume = %f", value);
    return value;
}

static void mute_each_sink(gpointer key, gpointer value, gpointer user_data)
{
    sink_info *info = (sink_info*)value;
    pa_operation_unref(pa_context_set_sink_mute_by_index(pulse_context, info->index, GPOINTER_TO_INT(user_data), context_success_callback,  NULL));
    g_debug("in the pulse manager: mute each sink %i", GPOINTER_TO_INT(user_data));
}

void toggle_global_mute(gboolean mute_value)
{
    g_hash_table_foreach(sink_hash, mute_each_sink, GINT_TO_POINTER(mute_value));
    g_debug("in the pulse manager: toggle global mute value %i", mute_value);
}


/*
Refine the resolution of the slider or binary scale it to achieve a more subtle volume control. 
Use the base volume stored in the sink struct to calculate actual linear volumes. 
*/
void set_sink_volume(gdouble percent)
{
    g_debug("in the pulse manager:set_sink_volume with percent %f", percent);
    if(DEFAULT_SINK_INDEX < 0)
    {
        g_warning("We have no default sink !!! - returning after not attempting to set any volume of any sink");
        return;
    }
    gdouble linear_input = (gdouble)(percent);
    linear_input /= 100.0;
    g_debug("linear double input = %f", linear_input);
    pa_volume_t new_volume = pa_sw_volume_from_linear(linear_input); 
    // Use this to achieve more accurate scaling using the base volume (in the sink struct already!)
    //pa_volume_t new_volume = (pa_volume_t) ((GPOINTER_TO_INT(linear_input) * s->base_volume) / 100);
    g_debug("about to try to set the sw volume to a linear volume of %f", pa_sw_volume_to_linear(new_volume));
    g_debug("and an actual volume of %f", (gdouble)new_volume);
    pa_cvolume dev_vol;
    sink_info *s = g_hash_table_lookup(sink_hash, GINT_TO_POINTER(DEFAULT_SINK_INDEX));   
    pa_cvolume_set(&dev_vol, s->volume.channels, new_volume);   
    
    pa_operation_unref(pa_context_set_sink_volume_by_index(pulse_context, DEFAULT_SINK_INDEX, &dev_vol, NULL, NULL));
}


/**********************************************************************************************************************/
//    Pulse-Audio asychronous call-backs
/**********************************************************************************************************************/

static void gather_pulse_information(pa_context *c, void *userdata)
{
    pa_operation *operation;
    if(!(operation = pa_context_get_server_info(c, pulse_server_info_callback, userdata)))
    {
        g_warning("pa_context_get_server_info failed");
        if (!(operation = pa_context_get_sink_info_list(c, pulse_sink_info_callback, NULL))) 
        {
            g_warning("pa_context_get_sink_info_list() failed - cannot fetch server or sink info - leaving . . .");
            return;
        }
    }
    pa_operation_unref(operation);
    return;
}


static void context_success_callback(pa_context *c, int success, void *userdata)
{
    g_debug("Context Success Callback - result = %i", success);
}

/**
On Service startup this callback will be called multiple times resulting our sinks_hash container to be filled with the
available sinks.
For now this callback it assumes it only used at startup. It may be necessary to use if sinks become available after startup.
Major candidate for refactoring.
**/
static void pulse_sink_info_callback(pa_context *c, const pa_sink_info *sink, int eol, void *userdata)
{
    if (eol > 0) {
        gboolean device_available = sink_available();
        if(device_available == TRUE)
        {
            // Hopefully the PA server has set the default device if not default to 0
            DEFAULT_SINK_INDEX = (DEFAULT_SINK_INDEX < 0) ? 0 : DEFAULT_SINK_INDEX;
            // TODO optimize
            // Cache method returns! (unneccessary multiple utility calls)
            // test_hash();
            update_pa_state(TRUE, device_available, default_sink_is_muted(), get_default_sink_volume()); 
            sound_service_dbus_update_sink_volume(dbus_service, get_default_sink_volume()); 
            sound_service_dbus_update_sink_mute(dbus_service, default_sink_is_muted()); 
            g_debug("default sink index : %d", DEFAULT_SINK_INDEX);                        
        }
        else{
            //Update the indicator to show PA either is not ready or has no available sink
            g_warning("Cannot find a suitable default sink ...");
            update_pa_state(FALSE, device_available, TRUE, 0); 
        }
    }
    else{
        g_debug("About to add an item to our hash");
        sink_info *value;
        value = g_new0(sink_info, 1);
        value->index = value->device_index = sink->index;
        value->name = g_strdup(sink->name);
        value->description = g_strdup(sink->description);
        value->icon_name = g_strdup(pa_proplist_gets(sink->proplist, PA_PROP_DEVICE_ICON_NAME));
        value->active_port = (sink->active_port != NULL);
        value->mute = !!sink->mute;
        value->volume = sink->volume;
        value->base_volume = sink->base_volume;
        value->channel_map = sink->channel_map;
        g_hash_table_insert(sink_hash, GINT_TO_POINTER(sink->index), value);
        g_debug("After adding an item to our hash");
    }
}

static void pulse_default_sink_info_callback(pa_context *c, const pa_sink_info *info, int eol, void *userdata)
{
    g_debug("default sink info callback");
    if (eol > 0) {        
        if (pa_context_errno(c) == PA_ERR_NOENTITY)
            return;
        g_warning("Default Sink info callback failure");
        return;
    }
    else{
        DEFAULT_SINK_INDEX = info->index;
        g_debug("Just set the default sink index to %i", DEFAULT_SINK_INDEX);    
        pa_operation_unref(pa_context_get_sink_info_list(c, pulse_sink_info_callback, NULL)); 
    }
}

static void pulse_sink_input_info_callback(pa_context *c, const pa_sink_input_info *info, int eol, void *userdata){
    if (eol > 0) {
        if (pa_context_errno(c) == PA_ERR_NOENTITY)
            return;
        g_warning("Sink INPUT info callback failure");
        return;
    }
	else{
        if (info == NULL)
        {
            // TODO: watch this carefully - PA async api should not be doing this . . .
            g_warning("\n Sink input info callback : SINK INPUT INFO IS NULL BUT EOL was not POSITIVE!!!");
            return;
        }
		g_debug("\n SINK INPUT INFO sink index : %d \n", info->sink);
        check_sink_input_while_muted_event(info->sink);
	}
}

static void update_sink_info(pa_context *c, const pa_sink_info *info, int eol, void *userdata)
{
    if (eol > 0) {
        if (pa_context_errno(c) == PA_ERR_NOENTITY)
            return;
        g_warning("Sink INPUT info callback failure");
        return;
    }

    GList *keys = g_hash_table_get_keys(sink_hash);
    gint position =  g_list_index(keys, GINT_TO_POINTER(info->index));
    if(position >= 0) // => index is within the keys of the hash.
    {
        sink_info *s = g_hash_table_lookup(sink_hash, GINT_TO_POINTER(info->index));
        //g_debug("attempting to update sink with name %s", s->name);
        s->name = g_strdup(info->name);
        s->description = g_strdup(info->description);
        s->icon_name = g_strdup(pa_proplist_gets(info->proplist, PA_PROP_DEVICE_ICON_NAME));
        s->active_port = (info->active_port != NULL);
        // NASTY!!
        gboolean mute_changed = s->mute != !!info->mute;
        s->mute = !!info->mute;
        s->volume = info->volume;
        s->base_volume = info->base_volume;
        s->channel_map = info->channel_map; 
        if(DEFAULT_SINK_INDEX == s->index)
        {
            //update the UI
            pa_volume_t vol = pa_cvolume_avg(&s->volume);
            // Use the base of the device to ensure maximum acceptable levels on the hardware
            gdouble volume_percent = (vol/s->base_volume) * 100;
            g_debug("When using base volume => volume = %f", volume_percent);
            g_debug("about to update ui with linear volume of %f", pa_sw_volume_to_linear(vol));            
            sound_service_dbus_update_sink_volume(dbus_service, pa_sw_volume_to_linear(vol)); 
            if (mute_changed == TRUE)     
                sound_service_dbus_update_sink_mute(dbus_service, s->mute);
            
            update_mute_ui(s->mute);
        }
    }
    else
    {
        // TODO ADD new sink - part of big refactor
        g_debug("attempting to add new sink with name %s", info->name);
        //sink_info *s;
        //s = g_new0(sink_info, 1);                
        //update the sinks hash with new sink.
    }    
}


static void pulse_server_info_callback(pa_context *c, const pa_server_info *info, void *userdata)
{
    g_debug("server info callback");
    pa_operation *operation;
    if (info == NULL)
    {
        g_warning("No server - get the hell out of here");
        update_pa_state(FALSE, FALSE, TRUE, 0); 
        pa_server_available = FALSE;
        return;    
    }
    pa_server_available = TRUE;
    if(info->default_sink_name != NULL)
    {
        if (!(operation = pa_context_get_sink_info_by_name(c, info->default_sink_name, pulse_default_sink_info_callback, userdata)))
        {
            g_warning("pa_context_get_sink_info_by_name() failed");
        }
        else{
            pa_operation_unref(operation);
            return;
        }
    }
    if (!(operation = pa_context_get_sink_info_list(c, pulse_sink_info_callback, NULL))) 
    {
        g_warning("pa_context_get_sink_info_list() failed");
        return;
    }             
    pa_operation_unref(operation);
}

static void subscribed_events_callback(pa_context *c, enum pa_subscription_event_type t, uint32_t index, void *userdata){
	switch (t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) {
        case PA_SUBSCRIPTION_EVENT_SINK:
            if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_REMOVE) {
                //TODO handle the remove event => if its our default sink - grey out the ui with update_pa_state
            } else {
                pa_operation_unref(pa_context_get_sink_info_by_index(c, index, update_sink_info, userdata));
            }            
            break;
        case PA_SUBSCRIPTION_EVENT_SINK_INPUT:
			// This will be triggered when the sink receives input from a new stream
			// If a playback client is paused and then resumed this will NOT trigger this event.
		    pa_operation_unref(pa_context_get_sink_input_info(c, index, pulse_sink_input_info_callback, userdata));
		    break;
        case PA_SUBSCRIPTION_EVENT_SERVER:
            if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_SINK ) {
                g_debug("server change of some sink type ???");
            }
	}
}


static void context_state_callback(pa_context *c, void *userdata) {
	switch (pa_context_get_state(c)) {
        case PA_CONTEXT_UNCONNECTED:
			g_debug("unconnected");
			break;
        case PA_CONTEXT_CONNECTING:
			g_debug("connecting");
			break;
        case PA_CONTEXT_AUTHORIZING:
			g_debug("authorizing");
			break;
        case PA_CONTEXT_SETTING_NAME:
			g_debug("context setting name");
			break;
        case PA_CONTEXT_FAILED:
			g_warning("FAILED to retrieve context - Is PulseAudio Daemon running ?");
            //Update the indicator to show PA either is not ready or has no available sink
            update_pa_state(FALSE, FALSE, TRUE, 0); 
			break;
        case PA_CONTEXT_TERMINATED:
			g_debug("context terminated");
			break;
        case PA_CONTEXT_READY:
			g_debug("PA daemon is ready");
            pa_operation *o;

            pa_context_set_subscribe_callback(c, subscribed_events_callback, userdata);

            if (!(o = pa_context_subscribe(c, (pa_subscription_mask_t)
                                           (PA_SUBSCRIPTION_MASK_SINK|
                                            PA_SUBSCRIPTION_MASK_SOURCE|
                                            PA_SUBSCRIPTION_MASK_SINK_INPUT|
                                            PA_SUBSCRIPTION_MASK_SOURCE_OUTPUT|
                                            PA_SUBSCRIPTION_MASK_CLIENT|
                                            PA_SUBSCRIPTION_MASK_SERVER|
                                            PA_SUBSCRIPTION_MASK_CARD), NULL, NULL))) {
                g_warning("pa_context_subscribe() failed");
                return;
            }
            pa_operation_unref(o);
            
            gather_pulse_information(c, userdata);

			break;
    }
}

