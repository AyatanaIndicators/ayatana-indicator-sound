/*
Tests for the libappindicator library that are over DBus.  This is
the client side of those tests.

Copyright 2010 Canonical Ltd.

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


#include <glib.h>
#include <dbus/dbus-glib.h>
#include "../src/dbus-shared-names.h"
/*#include "../src/indicator-sound.c"*/
#include "test-defines.h"

static GMainLoop * mainloop = NULL;
static gboolean passed = TRUE;

static void
fetch_mute_cb (DBusGProxy * proxy, DBusGProxyCall * call, void * data)
{
	GError * error = NULL;
	GValue value = {0};

	if (!dbus_g_proxy_end_call(proxy, call, &error, G_TYPE_VALUE, &value, G_TYPE_INVALID)) {
		g_warning("Getting mute failed: %s", error->message);
		g_error_free(error);
		passed = FALSE;
		return;
	}

	if (TEST_MUTE != g_value_get_boolean(&value))) {
		g_debug("Mute vale Returned: FAILED");
		passed = FALSE;
	} else {
		g_debug("Property ID Returned: PASSED");
	}
	return;
}


gboolean
kill_func (gpointer userdata)
{
	g_main_loop_quit(mainloop);
	g_warning("Forced to Kill");
	passed = FALSE;
	return FALSE;
}

gint
main (gint argc, gchar * argv[])
{
	g_type_init();

	g_usleep(500000);

	GError * error = NULL;
	DBusGConnection * session_bus = dbus_g_bus_get(DBUS_BUS_SESSION, &error);
	if (error != NULL) {
		g_error("Unable to get session bus: %s", error->message);
		return 1;
	}

	DBusGProxy * props = dbus_g_proxy_new_for_name_owner(session_bus,
														 INDICATOR_SOUND_DBUS_NAME,
														 INDICATOR_SOUND_SERVICE_DBUS_OBJECT,
														 INDICATOR_SOUND_SERVICE_DBUS_INTERFACE,
	                                                     &error);
	if (error != NULL) {
		g_error("Unable to get property proxy: %s", error->message);
		return 1;
	}

	dbus_g_proxy_begin_call (props,
	                         "GetSinkMute",
	                         fetch_mute_cb,
	                         NULL, NULL,
	                         G_TYPE_INVALID);

	g_timeout_add_seconds(2, kill_func, NULL);

	mainloop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(mainloop);

	if (passed) {
		g_debug("Quiting");
		return 0;
	} else {
		g_debug("Quiting as we're a failure");
		return 1;
	}
	return 0;
}
