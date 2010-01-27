/*
This service primarily controls PulseAudio and is driven by the sound indicator menu on the panel.
Copyright 2010 Canonical Ltd.

Authors:
    Conor Curran <conor.curran@canonical.com>
    Ted Gould <ted@canonical.com>
    Christoph Korn <c_korn@gmx.de>
    Cody Russell <crussell@canonical.com>

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

#include "sound-service-dbus.h" 
#include "sound-service.h"
#include "common-defs.h"

/**********************************************************************************************************************/
//    Pulse-Audio asychronous call-backs
/**********************************************************************************************************************/
static void context_get_sink_info_by_index_callback(pa_context *c, const pa_sink_info *sink, int eol, void *userdata){
    if (eol > 0) {
        return;
    }
	else{
		g_debug("\n SINK INFO Name : %s \n", sink->name);
		g_debug("\n SINK INFO Muted : %d \n", sink->mute);
		if (sink->mute == 1){
			g_debug("HERE is one for the DBUS - sink input while sink is muted");
            sound_service_dbus_sink_input_while_muted(dbus_interface, sink->index, TRUE);
		}
		else{
            g_debug("Sink input while the device is unmuted - not interested");
            sound_service_dbus_sink_input_while_muted(dbus_interface, sink->index, FALSE);
		}
	}
}

static void context_success_callback(pa_context *c, int success, void *userdata){
    g_debug("Context Success Callback - result = %i", success);
}

// TODO we are not handling multiple sinks appropriately
static void retrieve_complete_sink_list(pa_context *c, const pa_sink_info *sink, int eol, void *userdata){
    if(eol > 0){
        // TODO apparently never returns 0 sinks - Tested and it appears this assumption/prediction is correct.
        // i would imagine different behaviour on different machines - watch this space!
        // Some fuzzy reasoning might be needed.
        if(sink_list->len == 1){
            pa_sink_info* only_sink = (pa_sink_info*)g_ptr_array_index(sink_list, 0);
            //TODO: sink is not null but its module is the null-module-sink!
            // For now taking the easy route string compare on the name and the active port
            // needs more testing
            int value = g_strcasecmp(only_sink->name, " auto_null ");
            g_debug("comparison outcome with auto_null is %i", value);
            sink_available = (value != 0 && only_sink->active_port != NULL);
            // Strictly speaking all_muted should only be through if all sinks are muted
            // It is more application specific
            all_muted = (only_sink->mute == 1);
            g_debug("Available sink is named %s", only_sink->name);
            g_debug("does Available sink have an active port: %i", only_sink->active_port != NULL);
            g_debug("sink_available = %i", sink_available);
        }
        else{
            sink_available = TRUE;
        }
        // At this point we can be confident we know enough from PA to draw the UI
        rebuild_sound_menu (root_menuitem, dbus_interface);
    }
    else{
        g_ptr_array_add(sink_list, (gpointer)sink);
    }
}


static void set_global_mute_callback(pa_context *c, const pa_sink_info *sink, int eol, void *userdata){
    if(eol > 0){
        g_debug("No more sinks to mute ! \n Everything should now be muted/unmuted ?" );
        return;
    }
    // Otherwise mute/unmute it!
    pa_context_set_sink_mute_by_index(pulse_context, sink->index, all_muted == TRUE ? 1 : 0, context_success_callback, NULL);
}

static void context_get_sink_input_info_callback(pa_context *c, const pa_sink_input_info *info, int eol, void *userdata){
    if (eol > 0) {
        return;
    }
	else{
        if (info == NULL)
        {
            // TODO: watch this carefully - PA async api should not be doing this . . .
            g_debug("\n Sink input info callback : SINK INPUT INFO IS NULL BUT EOL was not POSITIVE!!!");
            return;
        }
        g_debug("\n SINK INPUT INFO CALLBACK about to start asking questions...\n");
		g_debug("\n SINK INPUT INFO Name : %s \n", info->name);
		g_debug("\n SINK INPUT INFO sink index : %d \n", info->sink);
		pa_operation_unref(pa_context_get_sink_info_by_index(c, info->sink, context_get_sink_info_by_index_callback, userdata));
	}
} 

static void subscribed_events_callback(pa_context *c, enum pa_subscription_event_type t, uint32_t index, void *userdata){
	switch (t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) {
        case PA_SUBSCRIPTION_EVENT_SINK:
            g_debug("Event sink for %i", index);
            break;
        case PA_SUBSCRIPTION_EVENT_SINK_INPUT:
			// This will be triggered when the sink receives input from a new stream
			// If a playback client is paused and then resumed this will NOT trigger this event.
			g_debug("Subscribed_events_callback - type = sink input and index = %i", index);
            g_debug("Sink input info query just about to happen");
			pa_operation_unref(pa_context_get_sink_input_info(c, index, context_get_sink_input_info_callback, userdata));
            g_debug("Sink input info query just happened");
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
			pa_context_set_subscribe_callback(c, subscribed_events_callback, userdata);		
			pa_operation_unref(pa_context_get_sink_info_list(c, retrieve_complete_sink_list, NULL));
			pa_operation_unref(pa_context_subscribe(c, PA_SUBSCRIPTION_MASK_SINK, NULL, NULL));
			pa_operation_unref(pa_context_subscribe(c, PA_SUBSCRIPTION_MASK_SINK_INPUT, NULL, NULL));
			break;
    }
}

/**********************************************************************************************************************/
//    Init functions (GTK and DBUS)
/**********************************************************************************************************************/
/**
Pass to the g_idle_add method - returning False will ensure that this method is never called again as it is removed as an event source. 
**/
static gboolean idle_routine (gpointer data)
{
    return FALSE;
}

/**
Build the DBus menu items. For now Mute all/Unmute is the only available option
**/
static void rebuild_sound_menu(DbusmenuMenuitem *root, SoundServiceDbus *service)
{
    mute_all_menuitem = dbusmenu_menuitem_new();

    dbusmenu_menuitem_property_set(mute_all_menuitem, DBUSMENU_MENUITEM_PROP_LABEL, _(all_muted == FALSE ? "Mute All" : "Unmute"));
    g_signal_connect(G_OBJECT(mute_all_menuitem), DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, G_CALLBACK(set_global_mute), NULL);
    //TODO: If no valid sinks are found grey out the item(s)
    dbusmenu_menuitem_property_set_bool(mute_all_menuitem, DBUSMENU_MENUITEM_PROP_SENSITIVE, sink_available);
    dbusmenu_menuitem_child_append(root, mute_all_menuitem);
}

static void set_global_mute()
{
    all_muted = !all_muted;
    g_debug("Mute is now = %i", all_muted);
	pa_operation_unref(pa_context_get_sink_info_list(pulse_context, set_global_mute_callback, NULL));
    dbusmenu_menuitem_property_set(mute_all_menuitem, DBUSMENU_MENUITEM_PROP_LABEL, _(all_muted == FALSE ? "Mute All" : "Unmute"));
}


/* When the service interface starts to shutdown, we
   should follow it. - 
*/
void
service_shutdown (IndicatorService *service, gpointer user_data)
{

	if (mainloop != NULL) {

/*		g_debug("Service shutdown");*/
/*        if (pulse_context){*/
/*    	    pa_context_unref(pulse_context);*/
/*    	}*/
/*        g_ptr_array_free(sink_list, TRUE);*/
/*        pa_glib_mainloop_free(pa_main_loop);*/
/*		g_main_loop_quit(mainloop);*/
	}
	return;
}


/* Main, is well, main.  It brings everything up and throws
   us into the mainloop of no return. Some refactoring needed.*/
int
main (int argc, char ** argv)
{
    g_type_init();

	setlocale (LC_ALL, "");
	bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
	textdomain (GETTEXT_PACKAGE);

	IndicatorService * service = indicator_service_new_version(INDICATOR_SOUND_DBUS_NAME,
	                                                           INDICATOR_SOUND_DBUS_VERSION);
	g_signal_connect(G_OBJECT(service),
	                 INDICATOR_SERVICE_SIGNAL_SHUTDOWN,
	                 G_CALLBACK(service_shutdown), NULL);    

    root_menuitem = dbusmenu_menuitem_new();
    g_debug("Root ID: %d", dbusmenu_menuitem_get_id(root_menuitem));
	
    g_idle_add(idle_routine, root_menuitem);

    sink_list = g_ptr_array_new();
    dbus_interface = g_object_new(SOUND_SERVICE_DBUS_TYPE, NULL);

    DbusmenuServer * server = dbusmenu_server_new(INDICATOR_SOUND_DBUS_OBJECT);
    dbusmenu_server_set_root(server, root_menuitem);

	pa_main_loop = pa_glib_mainloop_new(g_main_context_default());
    g_assert(pa_main_loop);
	pulse_context = pa_context_new(pa_glib_mainloop_get_api(pa_main_loop), "ayatana.indicator.sound");
	g_assert(pulse_context);


    // Establish event callback registration
	pa_context_set_state_callback(pulse_context, context_state_callback, NULL);
	pa_context_connect(pulse_context, NULL, PA_CONTEXT_NOAUTOSPAWN, NULL);

    //rebuild_sound_menu (root_menuitem, dbus_interface);

    // Run the loop
    mainloop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(mainloop);

    return 0;
}




