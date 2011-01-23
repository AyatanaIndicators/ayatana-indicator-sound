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

static gboolean b_startup = TRUE;

static gboolean idle_routine (gpointer data);
static void build_sound_menu(DbusmenuMenuitem *root,
                               gboolean mute_update,
                               gboolean availability,
                               gdouble volume);
static void show_sound_settings_dialog (DbusmenuMenuitem *mi,
                                        gpointer user_data);

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

/**
build_sound_menu's default items (without the any player items):
**/
static void build_sound_menu (DbusmenuMenuitem *root,
                              gboolean mute_update,
                              gboolean availability,
                              gdouble volume)
{
  // Mute button
  mute_menuitem = mute_menu_item_new ( mute_update, availability);
  dbusmenu_menuitem_child_append (root, DBUSMENU_MENUITEM(mute_menuitem));

  // Slider
  volume_slider_menuitem = slider_menu_item_new ( availability, volume );
  dbusmenu_menuitem_child_append (root, DBUSMENU_MENUITEM ( volume_slider_menuitem ));

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
update_pa_state:
**/
void dbus_menu_manager_update_pa_state (gboolean pa_state,
                                        gboolean pulse_available,
                                        gboolean sink_muted,
                                        gdouble percent)
{
  g_debug("update pa state with state %i, availability of %i, mute value of %i and a volume percent is %f", pa_state, pulse_available, sink_muted, percent);

  if (b_startup == TRUE) {
    build_sound_menu(root_menuitem, sink_muted, pulse_available, percent);
    b_startup = FALSE;
    return;
  }

  mute_menu_item_update ( mute_menuitem,
                          sink_muted );
  slider_menu_item_update ( volume_slider_menuitem,
                            percent );

  mute_menu_item_enable ( mute_menuitem, pulse_available);
  slider_menu_item_enable ( volume_slider_menuitem, 
                            pulse_available );

  // Emit the signals after the menus are setup/torn down
  // preserve ordering !
  /*sound_service_dbus_update_sink_availability(dbus_interface, sink_available);
  dbus_menu_manager_update_volume(percent);
  sound_service_dbus_update_sink_mute(dbus_interface, sink_muted);
  dbus_menu_manager_update_mute_ui(b_all_muted);*/
}

/**
update_mute_ui:
'public' method allowing the pa manager to update the mute menu item.
 These are wrappers until we figure out this refactor
**/
void dbus_menu_manager_update_mute(gboolean incoming_mute_value)
{      
  mute_menu_item_update (mute_menuitem, incoming_mute_value);
}

void dbus_menu_manager_update_volume(gdouble  volume)
{
  slider_menu_item_update (volume_slider_menuitem, volume);
}



/**
TODO: what are you doing with this
idle_routine:
Something for glip mainloop to do when idle
**/
static gboolean idle_routine (gpointer data)
{
  return FALSE;
}

