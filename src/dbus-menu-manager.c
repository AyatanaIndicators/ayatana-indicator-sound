/*
This service primarily controls PulseAudio and is driven by the sound indicator menu on the panel.
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


#include <unistd.h>
#include <glib/gi18n.h>

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

#include <libdbusmenu-glib/server.h>
#include <libdbusmenu-glib/menuitem.h>
#include <libdbusmenu-glib/client.h>

#include "dbus-menu-manager.h" 
#include "sound-service-dbus.h" 
#include "pulse-manager.h"
#include "slider-menu-item.h"

#include "dbus-shared-names.h"

// DBUS items
static DbusmenuMenuitem *root_menuitem = NULL;
static DbusmenuMenuitem *mute_all_menuitem = NULL;
static SliderMenuItem *volume_slider_menuitem = NULL;
static SoundServiceDbus *dbus_interface = NULL;

// PULSEAUDIO
static gboolean b_sink_available = FALSE;
static gboolean b_all_muted = FALSE;
static gboolean b_pulse_ready = FALSE;
static gboolean b_startup = TRUE;
static gdouble volume_percent = 0.0;

static void set_global_mute_from_ui();
static gboolean idle_routine (gpointer data);
static void rebuild_sound_menu(DbusmenuMenuitem *root, SoundServiceDbus *service);
static void refresh_menu();

/*-------------------------------------------------------------------------*/
//                          Public Methods 
/*-------------------------------------------------------------------------*/

/**
setup:
**/
void dbus_menu_manager_setup()
{
    root_menuitem = dbusmenu_menuitem_new();
    g_debug("Root ID: %d", dbusmenu_menuitem_get_id(root_menuitem));
	
    g_idle_add(idle_routine, root_menuitem);

    dbus_interface = g_object_new(SOUND_SERVICE_DBUS_TYPE, NULL);

    DbusmenuServer *server = dbusmenu_server_new(INDICATOR_SOUND_DBUS_OBJECT);
    dbusmenu_server_set_root(server, root_menuitem);
    establish_pulse_activities(dbus_interface);
}

/**
teardown:
**/
void dbus_menu_manager_teardown()
{
    //TODO tidy up dbus_interface and items!
}

/**
update_pa_state:
**/
void dbus_menu_manager_update_pa_state(gboolean pa_state, gboolean sink_available, gboolean sink_muted, gdouble percent)
{
    b_sink_available = sink_available;
    b_all_muted = sink_muted;
    b_pulse_ready = pa_state;
    volume_percent = percent;
	g_debug("update pa state with state %i, availability of %i, mute value of %i and a volume percent is %f", pa_state, sink_available, sink_muted, volume_percent);
    // Only rebuild the menu on start up...
    if(b_startup == TRUE){
        rebuild_sound_menu(root_menuitem, dbus_interface);
        b_startup = FALSE;
    }
    else{
        refresh_menu();
    }
    // Emit the signals after the menus are setup/torn down
    sound_service_dbus_update_sink_volume(dbus_interface, percent); 
    sound_service_dbus_update_sink_mute(dbus_interface, sink_muted); 
    dbus_menu_manager_update_mute_ui(b_all_muted);
}

/**
update_mute_ui:
'public' method allowing the pa manager to update the mute menu item.
**/
void dbus_menu_manager_update_mute_ui(gboolean incoming_mute_value)
{
    b_all_muted = incoming_mute_value;
    dbusmenu_menuitem_property_set(mute_all_menuitem,
                                    DBUSMENU_MENUITEM_PROP_LABEL, 
                                    _(b_all_muted == FALSE ? "Mute All" : "Unmute"));
}


/*-------------------------------------------------------------------------*/
//                          Private Methods 
/*-------------------------------------------------------------------------*/

static void refresh_menu()
{
    g_debug("in the refresh menu method");
    if(b_sink_available == FALSE || b_pulse_ready == FALSE)
    {

        dbusmenu_menuitem_property_set_bool(DBUSMENU_MENUITEM(volume_slider_menuitem), 
                                            DBUSMENU_MENUITEM_PROP_ENABLED,
                                            FALSE);   
        dbusmenu_menuitem_property_set_bool(DBUSMENU_MENUITEM(volume_slider_menuitem), 
                                            DBUSMENU_MENUITEM_PROP_VISIBLE,
                                            FALSE);   
        dbusmenu_menuitem_property_set_bool(mute_all_menuitem, 
                                            DBUSMENU_MENUITEM_PROP_ENABLED,
                                            FALSE);

    }
    else if(b_sink_available == TRUE  && b_pulse_ready == TRUE){

        dbusmenu_menuitem_property_set_bool(DBUSMENU_MENUITEM(volume_slider_menuitem), 
                                            DBUSMENU_MENUITEM_PROP_ENABLED,
                                            TRUE);   
        dbusmenu_menuitem_property_set_bool(DBUSMENU_MENUITEM(volume_slider_menuitem), 
                                            DBUSMENU_MENUITEM_PROP_VISIBLE,
                                            TRUE);   
        dbusmenu_menuitem_property_set_bool(mute_all_menuitem, 
                                            DBUSMENU_MENUITEM_PROP_ENABLED,
                                            TRUE);        
    }
}



/**

**/
static gboolean idle_routine (gpointer data)
{
    return FALSE;
}
 


/**
show_sound_settings_dialog:
Bring up the gnome volume preferences dialog
**/
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
rebuild_sound_menu:
Build the DBus menu items, mute/unmute, slider, separator and sound preferences 'link'
**/
static void rebuild_sound_menu(DbusmenuMenuitem *root, SoundServiceDbus *service)
{
    // Mute button
    mute_all_menuitem = dbusmenu_menuitem_new();
    dbusmenu_menuitem_property_set(mute_all_menuitem, DBUSMENU_MENUITEM_PROP_LABEL, _(b_all_muted == FALSE ? "Mute All" : "Unmute"));
    g_signal_connect(G_OBJECT(mute_all_menuitem), DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, G_CALLBACK(set_global_mute_from_ui), NULL);
    dbusmenu_menuitem_property_set_bool(mute_all_menuitem, DBUSMENU_MENUITEM_PROP_ENABLED, b_sink_available);

    // Slider
    volume_slider_menuitem = slider_menu_item_new(b_sink_available, volume_percent);
    dbusmenu_menuitem_child_append(root, mute_all_menuitem);
    dbusmenu_menuitem_child_append(root, DBUSMENU_MENUITEM(volume_slider_menuitem));
    dbusmenu_menuitem_property_set_bool(DBUSMENU_MENUITEM(volume_slider_menuitem),
                                        DBUSMENU_MENUITEM_PROP_ENABLED,
                                        b_sink_available);       
    dbusmenu_menuitem_property_set_bool(DBUSMENU_MENUITEM(volume_slider_menuitem),
                                        DBUSMENU_MENUITEM_PROP_VISIBLE,
                                        b_sink_available);   
    // Separator
    DbusmenuMenuitem *separator = dbusmenu_menuitem_new();
    dbusmenu_menuitem_property_set(separator, DBUSMENU_MENUITEM_PROP_TYPE, DBUSMENU_CLIENT_TYPES_SEPARATOR);
    dbusmenu_menuitem_child_append(root, separator);

    // Sound preferences dialog
    DbusmenuMenuitem *settings_mi = dbusmenu_menuitem_new();
    dbusmenu_menuitem_property_set(settings_mi, DBUSMENU_MENUITEM_PROP_LABEL,
                                                                   _("Sound Preferences..."));
    dbusmenu_menuitem_child_append(root, settings_mi);
    g_signal_connect(G_OBJECT(settings_mi), DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,
                                         G_CALLBACK(show_sound_settings_dialog), NULL);
}

/**
set_global_mute_from_ui:
Callback for the dbusmenuitem button
**/
static void set_global_mute_from_ui()
{
    b_all_muted = !b_all_muted;
    toggle_global_mute(b_all_muted); 
    dbusmenu_menuitem_property_set(mute_all_menuitem,
                                   DBUSMENU_MENUITEM_PROP_LABEL,
                                   _(b_all_muted == FALSE ? "Mute All" : "Unmute"));
}


