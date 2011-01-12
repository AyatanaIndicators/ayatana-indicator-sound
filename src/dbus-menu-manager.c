/*
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

/**
 *TODO: 
 * Makes this a proper GObject
 **/

#include <unistd.h>
#include <glib/gi18n.h>

#include <libdbusmenu-glib/server.h>
#include <libdbusmenu-glib/client.h>

#include "dbus-menu-manager.h"
#include "sound-service-dbus.h"
#include "pulse-manager.h"
#include "slider-menu-item.h"
#include "mute-menu-item.h"

#include "common-defs.h"

#include "dbus-shared-names.h"

// DBUS related
static DbusmenuMenuitem *root_menuitem = NULL;
static SliderMenuItem *volume_slider_menuitem = NULL;
static MuteMenuItem *mute_menuitem = NULL;
static SoundServiceDbus *dbus_interface = NULL;

// PULSEAUDIO
static gboolean b_sink_available = FALSE;
static gboolean b_pulse_ready = FALSE;
static gboolean b_startup = TRUE;
static gdouble volume_percent = 0.0;

static void set_global_mute_from_ui();
static gboolean idle_routine (gpointer data);
static void rebuild_sound_menu(DbusmenuMenuitem *root,
                               gboolean mute_update,
                               gboolean availability,
                               gdouble volume);
static void refresh_menu();


/*-------------------------------------------------------------------------*/
//                          Public Methods
/*-------------------------------------------------------------------------*/

/**
setup:
**/
DbusmenuMenuitem* dbus_menu_manager_setup()
{
  root_menuitem = dbusmenu_menuitem_new();
  g_debug("Root ID: %d", dbusmenu_menuitem_get_id(root_menuitem));

  g_idle_add(idle_routine, root_menuitem);

  dbus_interface = g_object_new(SOUND_SERVICE_DBUS_TYPE, NULL);

  DbusmenuServer *server = dbusmenu_server_new(INDICATOR_SOUND_MENU_DBUS_OBJECT_PATH);
  dbusmenu_server_set_root(server, root_menuitem);
  establish_pulse_activities(dbus_interface);
  return root_menuitem;
}

void dbus_menu_manager_update_volume(gdouble  volume)
{
  GVariant* new_volume = g_variant_new_double(volume);
  dbusmenu_menuitem_property_set_variant(DBUSMENU_MENUITEM(volume_slider_menuitem),
                                         DBUSMENU_VOLUME_MENUITEM_LEVEL,
                                         new_volume);  
}
                                     

/**
update_pa_state:
**/
void dbus_menu_manager_update_pa_state (gboolean pa_state,
                                        gboolean sink_available,
                                        gboolean sink_muted,
                                        gdouble percent)
{
  b_sink_available = sink_available;
  b_pulse_ready = pa_state;
  volume_percent = percent;
  g_debug("update pa state with state %i, availability of %i, mute value of %i and a volume percent is %f", pa_state, sink_available, sink_muted, volume_percent);
  // Only rebuild the menu on start up...
  if (b_startup == TRUE) {
    rebuild_sound_menu(root_menuitem, dbus_interface);
    b_startup = FALSE;
  } else {
    refresh_menu();
  }
  // Emit the signals after the menus are setup/torn down
  // preserve ordering !
  sound_service_dbus_update_sink_availability(dbus_interface, sink_available);
  dbus_menu_manager_update_volume(percent);
  sound_service_dbus_update_sink_mute(dbus_interface, sink_muted);
  dbus_menu_manager_update_mute_ui(b_all_muted);
}

/**
update_mute_ui:
'public' method allowing the pa manager to update the mute menu item.
**/
void dbus_menu_manager_update_mute_ui(gboolean incoming_mute_value)
{
      
  dbusmenu_menuitem_property_set(DBUSMENUITEM(mute_all_menuitem),
                                 DBUSMENU_MENUITEM_PROP_LABEL,
                                 b_all_muted == FALSE ? _("Mute") : _("Unmute"));
}


/*-------------------------------------------------------------------------*/
//                          Private Methods
/*-------------------------------------------------------------------------*/
static void refresh_menu()
{
  g_debug("in the refresh menu method");
  if (b_sink_available == FALSE || b_pulse_ready == FALSE) {

    dbusmenu_menuitem_property_set_bool(DBUSMENU_MENUITEM(volume_slider_menuitem),
                                        DBUSMENU_MENUITEM_PROP_ENABLED,
                                        FALSE);
    dbusmenu_menuitem_property_set_bool(DBUSMENU_MENUITEM(volume_slider_menuitem),
                                        DBUSMENU_MENUITEM_PROP_VISIBLE,
                                        FALSE);
    dbusmenu_menuitem_property_set_bool(mute_all_menuitem,
                                        DBUSMENU_MENUITEM_PROP_ENABLED,
                                        FALSE);

  } else if (b_sink_available == TRUE  && b_pulse_ready == TRUE) {

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
idle_routine:
Something for glip mainloop to do when idle
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
  if (!g_spawn_command_line_async("gnome-volume-control --page=applications", &error) &&
      !g_spawn_command_line_async("xfce4-mixer", &error)) 
  {
    g_warning("Unable to show dialog: %s", error->message);
    g_error_free(error);
  }
}

/**
build_sound_menu's default items (without the any player items):
**/
static void build_sound_menu (DbusmenuMenuitem *root,
                              gboolean mute_update,
                              gboolean availability,
                              gdouble volume);

{
  // Mute button
  mute_all_menuitem = mute_menu_item_new ( mute_update, availability);
  dbusmenu_menuitem_child_append(root, mute_all_menuitem);

  // Slider
  volume_slider_menuitem = slider_menu_item_new(available, volume);
  dbusmenu_menuitem_child_append(root, DBUSMENU_MENUITEM(volume_slider_menuitem));
  dbusmenu_menuitem_property_set_bool(DBUSMENU_MENUITEM(volume_slider_menuitem),
                                      DBUSMENU_MENUITEM_PROP_ENABLED,
                                      b_sink_available && !b_all_muted);
  dbusmenu_menuitem_property_set_bool(DBUSMENU_MENUITEM(volume_slider_menuitem),
                                      DBUSMENU_MENUITEM_PROP_VISIBLE,
                                      b_sink_available);
  // Separator
  DbusmenuMenuitem *separator = dbusmenu_menuitem_new();
  dbusmenu_menuitem_property_set( separator,
                                  DBUSMENU_MENUITEM_PROP_TYPE,
                                  DBUSMENU_CLIENT_TYPES_SEPARATOR);
  dbusmenu_menuitem_child_append(root, separator);

  // Sound preferences dialog
  DbusmenuMenuitem *settings_mi = dbusmenu_menuitem_new();
  dbusmenu_menuitem_property_set( settings_mi,
                                  DBUSMENU_MENUITEM_PROP_LABEL,
                                  _("Sound Preferences..."));
  
  //_("Sound Preferences..."));
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
  dbusmenu_menuitem_property_set((DBUSMENU_MENUITEM)mute_all_menuitem,
                                  DBUSMENU_MENUITEM_PROP_LABEL,
                                  b_all_muted == FALSE ? _("Mute") : _("Unmute"));
}


/*
 TODO: use these temporary wrappers around pulsemanager for the short term
 Until I get to the point where I can refactor it entirely.
*/

void dbmm_pa_wrapper_toggle_mute(gboolean update)
{
  toggle_global_mute (update);
}


