#ifndef __INCLUDE_DBUS_MENU_MANAGER_H__
#define __INCLUDE_DBUS_MENU_MANAGER_H__

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

#include <libdbusmenu-glib/menuitem.h>

// Entry point
DbusmenuMenuitem* dbus_menu_manager_setup();

void dbus_menu_manager_update_pa_state (gboolean pa_state,
                                        gboolean sink_available,
                                        gboolean sink_muted,
                                        gdouble current_vol);

// Temporary wrappers on the pulsemanager inward calls
// until the refactor is complete
void dbus_menu_manager_update_mute (gboolean incoming_mute_value);
void dbus_menu_manager_update_volume (gdouble  volume);
#endif

