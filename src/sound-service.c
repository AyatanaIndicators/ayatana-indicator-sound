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
#include "sound-service.h"
#include "sound-service-dbus.h" 
#include "pulse-manager.h"
#include "slider-menu-item.h"
#include "common-defs.h"


// GTK + DBUS
static GMainLoop *mainloop = NULL;
static DbusmenuMenuitem *root_menuitem = NULL;
static DbusmenuMenuitem *mute_all_menuitem = NULL;
static SliderMenuItem *volume_slider_menuitem = NULL;
static SoundServiceDbus *dbus_interface = NULL;

// PULSEAUDIO
static gboolean b_sink_available = FALSE;
static gboolean b_all_muted = FALSE;
static gboolean b_pulse_ready = FALSE;
static gdouble volume_percent = 0.0;

static void set_global_mute();
static gboolean idle_routine (gpointer data);
static void rebuild_sound_menu(DbusmenuMenuitem *root, SoundServiceDbus *service);


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
 

static void show_sound_settings_dialog (DbusmenuMenuitem *mi, gpointer user_data) 
{
    GError * error = NULL;
    if (!g_spawn_command_line_async("gnome-volume-control", &error)) 
    {
        g_warning("Unable to show dialog: %s", error->message);
        g_error_free(error);
    }
}
/**
Build the DBus menu items. For now Mute all/Unmute is the only available option
**/
static void rebuild_sound_menu(DbusmenuMenuitem *root, SoundServiceDbus *service)
{
    // Mute button
    mute_all_menuitem = dbusmenu_menuitem_new();
    dbusmenu_menuitem_property_set(mute_all_menuitem, DBUSMENU_MENUITEM_PROP_LABEL, _(b_all_muted == FALSE ? "Mute All" : "Unmute"));
    g_signal_connect(G_OBJECT(mute_all_menuitem), DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, G_CALLBACK(set_global_mute), NULL);
    //TODO: If no valid sinks are found grey out the item(s)
    dbusmenu_menuitem_property_set_bool(mute_all_menuitem, DBUSMENU_MENUITEM_PROP_SENSITIVE, b_sink_available);

    // Slider
    volume_slider_menuitem = slider_menu_item_new(b_sink_available, volume_percent);
    dbusmenu_menuitem_child_append(root, mute_all_menuitem);
    dbusmenu_menuitem_child_append(root, DBUSMENU_MENUITEM(volume_slider_menuitem));

    DbusmenuMenuitem *separator = dbusmenu_menuitem_new();
    dbusmenu_menuitem_property_set(separator, DBUSMENU_MENUITEM_PROP_TYPE, DBUSMENU_CLIENT_TYPES_SEPARATOR);
    dbusmenu_menuitem_child_append(root, separator);
    DbusmenuMenuitem *settings_mi = dbusmenu_menuitem_new();
    dbusmenu_menuitem_property_set(settings_mi, DBUSMENU_MENUITEM_PROP_LABEL,
                                                                   _("Sound Preferences..."));
    dbusmenu_menuitem_child_append(root, settings_mi);
    g_signal_connect(G_OBJECT(settings_mi), DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,
                                         G_CALLBACK(show_sound_settings_dialog), NULL);
}

static void set_global_mute()
{
    b_all_muted = !b_all_muted;
    toggle_global_mute(b_all_muted); 
    dbusmenu_menuitem_property_set(mute_all_menuitem, DBUSMENU_MENUITEM_PROP_LABEL, _(b_all_muted == FALSE ? "Mute All" : "Unmute"));

/*    GValue value = {0};*/
/*    g_value_init(&value, G_TYPE_DOUBLE);*/
/*    g_value_set_double(&value, 100.0);*/
/*    // Testing*/
/*    dbusmenu_menuitem_property_set_value(DBUSMENU_MENUITEM(volume_slider_menuitem), DBUSMENU_SLIDER_MENUITEM_PROP_VOLUME, &value);*/
}


/* When the service interface starts to shutdown, we
   should follow it. - 
*/
void
service_shutdown (IndicatorService *service, gpointer user_data)
{

	if (mainloop != NULL) {

		g_debug("Service shutdown - but commented out for right now");
/*        close_pulse_activites()*/
/*		  g_main_loop_quit(mainloop);*/
	}
	return;
}

void update_pa_state(gboolean pa_state, gboolean sink_available, gboolean sink_muted, gdouble percent)
{
    b_sink_available = sink_available;
    b_all_muted = sink_muted;
    b_pulse_ready = pa_state;
    volume_percent = percent;
	g_debug("update pa state with state %i, availability of %i, mute value of %i and a volume percent is %f", pa_state, sink_available, sink_muted, volume_percent);
    rebuild_sound_menu(root_menuitem, dbus_interface);
    g_debug("About to send over the volume slider");
    GValue value = {0};
    g_value_init(&value, G_TYPE_DOUBLE);
    g_value_set_double(&value, volume_percent * 100);
    dbusmenu_menuitem_property_set_value(DBUSMENU_MENUITEM(volume_slider_menuitem), DBUSMENU_SLIDER_MENUITEM_PROP_VOLUME, &value);
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

	IndicatorService *service = indicator_service_new_version(INDICATOR_SOUND_DBUS_NAME,
	                                                           INDICATOR_SOUND_DBUS_VERSION);
	g_signal_connect(G_OBJECT(service),
	                 INDICATOR_SERVICE_SIGNAL_SHUTDOWN,
	                 G_CALLBACK(service_shutdown), NULL);    

    root_menuitem = dbusmenu_menuitem_new();
    g_debug("Root ID: %d", dbusmenu_menuitem_get_id(root_menuitem));
	
    g_idle_add(idle_routine, root_menuitem);

    dbus_interface = g_object_new(SOUND_SERVICE_DBUS_TYPE, NULL);

    DbusmenuServer *server = dbusmenu_server_new(INDICATOR_SOUND_DBUS_OBJECT);
    dbusmenu_server_set_root(server, root_menuitem);
    establish_pulse_activities(dbus_interface);

    // Run the loop
    mainloop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(mainloop);

    return 0;
}




