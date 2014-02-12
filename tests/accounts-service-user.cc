/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *      Ted Gould <ted@canonical.com>
 */

#include <gtest/gtest.h>
#include <gio/gio.h>
#include <libdbustest/dbus-test.h>
#include <act/act.h>

extern "C" {
#include "indicator-sound-service.h"
}

class AccountsServiceUserTest : public ::testing::Test
{

	protected:
		DbusTestService * service = NULL;

		GDBusConnection * session = NULL;
		GDBusConnection * system = NULL;

		virtual void SetUp() {
			service = dbus_test_service_new(NULL);
			dbus_test_service_start_tasks(service);

			g_setenv("DBUS_SYSTEM_BUS_ADDRESS", g_getenv("DBUS_SESSION_BUS_ADDRESS"), TRUE);

			session = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
			g_dbus_connection_set_exit_on_close(session, FALSE);
			g_object_add_weak_pointer(G_OBJECT(session), (gpointer *)&session);

			system = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, NULL);
			g_dbus_connection_set_exit_on_close(system, FALSE);
			g_object_add_weak_pointer(G_OBJECT(system), (gpointer *)&system);
		}

		virtual void TearDown() {
			/* These are the things that libaccountservice0 doesn't clean up :-( */
			g_object_unref(act_user_manager_get_default());
			g_object_unref(system);
			g_object_unref(system);
			g_object_unref(system);
			/* End shitty untested library cleanup */

			g_clear_object(&service);

			g_object_unref(session);
			g_object_unref(system);

			unsigned int cleartry = 0;
			while ((session != NULL || system != NULL) && cleartry < 100) {
				loop(100);
				cleartry++;
			}

			ASSERT_EQ(nullptr, session);
			ASSERT_EQ(nullptr, system);
		}

		static gboolean timeout_cb (gpointer user_data) {
			GMainLoop * loop = static_cast<GMainLoop *>(user_data);
			g_main_loop_quit(loop);
			return G_SOURCE_REMOVE;
		}

		void loop (unsigned int ms) {
			GMainLoop * loop = g_main_loop_new(NULL, FALSE);
			g_timeout_add(ms, timeout_cb, loop);
			g_main_loop_run(loop);
			g_main_loop_unref(loop);
		}
};

TEST_F(AccountsServiceUserTest, BasicObject) {
	AccountsServiceUser * srv = accounts_service_user_new();
	loop(50);
	g_object_unref(srv);
}
