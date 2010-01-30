#include <pulse/glib-mainloop.h>
#include <pulse/error.h>
#include <pulse/gccmacro.h>

#include "pulse-manager.h"
#include "sound-service.h"


static pa_context *pulse_context = NULL;
static pa_glib_mainloop *pa_main_loop = NULL;
static void context_state_callback(pa_context *c, void *userdata);
static GHashTable *sink_hash = NULL;
static SoundServiceDbus *dbus_service = NULL;

static void context_state_callback(pa_context *c, void *userdata);
static void pulse_sink_info_callback(pa_context *c, const pa_sink_info *sink_info, int eol, void *userdata);
static void context_success_callback(pa_context *c, int success, void *userdata);

pa_context* get_context(void) 
{
  return pulse_context;
}

void set_sink_volume(gint sink_index, gint percent)
{
    g_debug("in the pulse manager:set_sink_volume with index %i and percent %i", sink_index, percent);
}

void establish_pulse_activities(SoundServiceDbus *service)
{
    dbus_service = service;
	pa_main_loop = pa_glib_mainloop_new(g_main_context_default());
    g_assert(pa_main_loop);
	pulse_context = pa_context_new(pa_glib_mainloop_get_api(pa_main_loop), "ayatana.indicator.sound");
	g_assert(pulse_context);
    sink_hash = g_hash_table_new_full(g_int_hash, g_int_equal, g_free, g_free);
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
}

static void mute_each_sink(gpointer key, gpointer value, gpointer user_data)
{
//mute_value == TRUE ? 1 : 0,
    sink_info *info = (sink_info*)value;
    pa_operation_unref(pa_context_set_sink_mute_by_index(pulse_context, info->index, GPOINTER_TO_INT(user_data), context_success_callback,  NULL));
}

void toggle_global_mute(gboolean mute_value)
{
    g_hash_table_foreach(sink_hash, mute_each_sink, GINT_TO_POINTER(mute_value));
}

void set_volume(gint sink_index, gint volume_percent)
{
    g_debug("set_volume in the sound-service");
}


static void test_hash(){
    guint size = 0;
    size = g_hash_table_size(sink_hash);
    g_debug("Size of hash = %i", size);
    gint *key;
    key = g_new(gint, 1);
    *key = 0;
    sink_info *s = g_hash_table_lookup(sink_hash, key);   
    g_debug("and the name of our sink is %s", s->name); 
}

static gboolean sink_available()
{
    if (g_hash_table_size(sink_hash) < 1)
        return FALSE;
    int *key;
    key = g_new(gint, 1);
    *key = 0;
    sink_info *s = g_hash_table_lookup(sink_hash, key);   
    //int value = g_strcasecmp(s->name, " auto_null ");
    return ((g_strcasecmp(s->name, " auto_null ") != 0) && s->active_port == TRUE);
}

// We are assuming the device is 0 for now. 
static gboolean chosen_sink_is_muted()
{
    if (g_hash_table_size(sink_hash) < 1)
        return FALSE;
    int *key;
    key = g_new(gint, 1);
    *key = 0;
    sink_info *s = g_hash_table_lookup(sink_hash, key);   
    return s->mute;
}


/**********************************************************************************************************************/
//    Pulse-Audio asychronous call-backs
/**********************************************************************************************************************/
/*static void pulse_audio_server_info_callback(pa_context *c, const pa_server_info *server_info, void *userdata) */
/*{*/

/*}*/

static void gather_pulse_information(pa_context *c, void *userdata)
{
    pa_operation *operation;

    if (!(operation = pa_context_get_sink_info_list(c, pulse_sink_info_callback, NULL))) 
    {
        g_warning("pa_context_get_sink_info_list() failed");
        return;
    }
    pa_operation_unref(operation);

}


static void context_success_callback(pa_context *c, int success, void *userdata)
{
    g_debug("Context Success Callback - result = %i", success);
}


static void pulse_sink_info_callback(pa_context *c, const pa_sink_info *sink, int eol, void *userdata)
{
    if (eol > 0) {
        test_hash();
        update_pa_state(TRUE, sink_available(), chosen_sink_is_muted()); 
        
        // TODO follow this pattern for all other async call-backs involving lists - safest/most accurate approach.
        if (pa_context_errno(c) == PA_ERR_NOENTITY)
            return;
        g_warning("Sink info callback failure");
        return;
    }
    else{
        gint *key;
        key = g_new(gint, 1);
        *key = sink->index;
        sink_info *value;
        value = g_new(sink_info, 1);
        value->index = value->device_index = sink->index;
        value->name = sink->name;
        value->description = sink->description;
        value->icon_name = pa_proplist_gets(sink->proplist, PA_PROP_DEVICE_ICON_NAME);
        value->active_port = (sink->active_port != NULL);
        value->mute = !!sink->mute;
        g_hash_table_insert(sink_hash, key, value);
        // VOLUME focus tmrw.
        //value->volume = sink->volume;
        //value->channel_map = sink->channel_map;
    }
}


/*static void context_get_sink_input_info_callback(pa_context *c, const pa_sink_input_info *info, int eol, void *userdata){*/
/*    if (eol > 0) {*/
/*        return;*/
/*    }*/
/*	else{*/
/*        if (info == NULL)*/
/*        {*/
/*            // TODO: watch this carefully - PA async api should not be doing this . . .*/
/*            g_debug("\n Sink input info callback : SINK INPUT INFO IS NULL BUT EOL was not POSITIVE!!!");*/
/*            return;*/
/*        }*/
/*        g_debug("\n SINK INPUT INFO CALLBACK about to start asking questions...\n");*/
/*		g_debug("\n SINK INPUT INFO Name : %s \n", info->name);*/
/*		g_debug("\n SINK INPUT INFO sink index : %d \n", info->sink);*/
/*		pa_operation_unref(pa_context_get_sink_info_by_index(c, info->sink, context_get_sink_info_by_index_callback, userdata));*/
/*	}*/
/*} */

static void subscribed_events_callback(pa_context *c, enum pa_subscription_event_type t, uint32_t index, void *userdata){
	switch (t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) {
        case PA_SUBSCRIPTION_EVENT_SINK:
            //g_debug("Event sink for %i", index);
            break;
        case PA_SUBSCRIPTION_EVENT_SINK_INPUT:
			// This will be triggered when the sink receives input from a new stream
			// If a playback client is paused and then resumed this will NOT trigger this event.
			//g_debug("Subscribed_events_callback - type = sink input and index = %i", index);
            //g_debug("Sink input info query just about to happen");
			//pa_operation_unref(pa_context_get_sink_input_info(c, index, context_get_sink_input_info_callback, userdata));
            //g_debug("Sink input info query just happened");
		    break;
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
			g_debug("FAILED to retrieve context");
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

