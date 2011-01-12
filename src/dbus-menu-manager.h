#ifndef __INCLUDE_DBUS_MENU_MANAGER_H__
#define __INCLUDE_DBUS_MENU_MANAGER_H__

#include <libdbusmenu-glib/menuitem.h>

/*
This handles the management of the dbusmeneu items.
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

DbusmenuMenuitem* dbus_menu_manager_setup();
void dbus_menu_manager_teardown();
void dbus_menu_manager_update_volume(gdouble volume);
void dbus_menu_manager_update_pa_state(gboolean pa_state, gboolean sink_available, gboolean sink_muted, gdouble current_vol);
// TODO update pa_state should incorporate the method below !
void dbus_menu_manager_update_mute_ui(gboolean incoming_mute_value);
void dbmm_pa_wrapper_toggle_mute(gboolean update);
#endif

