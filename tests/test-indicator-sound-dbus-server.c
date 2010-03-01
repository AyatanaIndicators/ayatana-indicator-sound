/*
Tests for the libappindicator library that are over DBus.  This is
the server side of those tests.

Copyright 2009 Canonical Ltd.

Authors:
    Conor Curran <conor.curran@canonical.com>
    Ted Gould <ted@canonical.com>

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


#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <glib.h>
#include "../src/sound-service-dbus.h"
#include "test-defines.h"

static GMainLoop * mainloop = NULL;

gboolean
kill_func (gpointer userdata)
{
	g_main_loop_quit(mainloop);
	return FALSE;
}

gint
main (gint argc, gchar * argv[])
{
    g_type_init();
	g_debug("DBus ID: %s", dbus_connection_get_server_id(dbus_g_connection_get_connection(dbus_g_bus_get(DBUS_BUS_SESSION, NULL))));

    dbus_interface = g_object_new(SOUND_SERVICE_DBUS_TYPE, NULL);

    // Set the mute value
    sound_service_dbus_update_sink_mute(dbus_interface, TEST_MUTE);
    g_timeout_add_seconds(2, kill_func, NULL);

    // Run the loop
    mainloop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(mainloop);

	g_debug("Quiting");

    return 0;
}


