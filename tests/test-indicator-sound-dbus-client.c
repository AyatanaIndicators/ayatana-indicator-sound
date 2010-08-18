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
#include "test-defines.h"
#include "../src/sound-service-client.h"

static GMainLoop * mainloop = NULL;
static DBusGProxy * proxy= NULL;

static void
test_fetch_mute(DBusGProxy * proxy)
{
  GError * error = NULL;
  gboolean *fetched_mute_value;
  fetched_mute_value = g_new0(gboolean, 1);
  org_ayatana_indicator_sound_get_sink_mute(proxy, fetched_mute_value, &error);
	if (error != NULL) {
		g_warning("test-indicator-sound-dbus-client::test_fetch_mute - Unable to fetch mute: %s", error->message);
		g_error_free(error);
    g_free(fetched_mute_value);
    return;
	}
  g_assert(TEST_MUTE_VALUE == *fetched_mute_value); 
  g_free(fetched_mute_value);
}

static void 
test_fetch_availability(DBusGProxy * proxy)
{
	GError * error = NULL;
	gboolean * available_input;
	available_input = g_new0(gboolean, 1);
	org_ayatana_indicator_sound_get_sink_availability(proxy, available_input, &error);
	if (error != NULL) {
		g_warning("test-indicator-sound-dbus-client::test_fetch_availability - unable to fetch availability %s", error->message);
		g_error_free(error);
	  g_free(available_input);
	  return;
	}
	g_assert(TEST_AVAILABLE_VALUE == *available_input);
	g_free(available_input);
}


gboolean
kill_func (gpointer userdata)
{
  g_free(proxy);
	g_main_loop_quit(mainloop);
	return FALSE;
}

gint
main (gint argc, gchar * argv[])
{
	g_type_init();
	g_test_init(&argc, &argv, NULL);

	g_usleep(500000);

	GError * error = NULL;
	DBusGConnection * session_bus = dbus_g_bus_get(DBUS_BUS_SESSION, &error);
	if (error != NULL) {
		g_error("Unable to get session bus: %s", error->message);
		return 1;
	}

	DBusGProxy * proxy = dbus_g_proxy_new_for_name_owner(session_bus,
                                            					 ":1.0",
											 																	INDICATOR_SOUND_SERVICE_DBUS_OBJECT,
                                             						INDICATOR_SOUND_SERVICE_DBUS_INTERFACE,
                                             						&error);
	if (error != NULL) {
		g_error("Unable to get property proxy: %s", error->message);
		return 1;
	}

  test_fetch_mute(proxy);
  test_fetch_availability(proxy);    

	g_timeout_add_seconds(2, kill_func, NULL);

	mainloop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(mainloop);

	return 0;
}
