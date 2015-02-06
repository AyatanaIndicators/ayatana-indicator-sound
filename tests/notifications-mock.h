/*
 * Copyright © 2015 Canonical Ltd.
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

#include <libdbustest/dbus-test.h>

class NotificationsMock
{
		DbusTestDbusMock * mock = nullptr;

	public:
		NotificationsMock (std::vector<std::string> capabilities = {"body", "body-markup", "icon-static", "image/svg+xml", "x-canonical-private-synchronous", "x-canonical-append", "x-canonical-private-icon-only", "x-canonical-truncation", "private-synchronous", "append", "private-icon-only", "truncation"}) {
			mock = dbus_test_dbus_mock_new("org.freedesktop.Notifications");
			dbus_test_task_set_bus(DBUS_TEST_TASK(mock), DBUS_TEST_SERVICE_BUS_SESSION);
			dbus_test_task_set_name(DBUS_TEST_TASK(mock), "Notify");

			DbusTestDbusMockObject * baseobj =dbus_test_dbus_mock_get_object(mock, "/org/freedesktop/Notifications", "org.freedesktop.Notifications", NULL);

			std::string capspython("ret = ");
			capspython += vector2py(capabilities);
			dbus_test_dbus_mock_object_add_method(mock, baseobj,
				"GetCapabilities", NULL, G_VARIANT_TYPE("as"),
				capspython.c_str(), NULL);
		}

		~NotificationsMock () {
			g_debug("Destroying the Accounts Service Mock");
			g_clear_object(&mock);
		}

		std::string vector2py (std::vector<std::string> vect) {
			std::string retval("[ ");

			std::for_each(vect.begin(), vect.end() - 1, [&retval](std::string entry) {
				retval += "'";
				retval += entry;
				retval += "', ";
			});

			retval += "'";
			retval += *(vect.end() - 1);
			retval += "']";

			return retval;
		}

		operator std::shared_ptr<DbusTestTask> () {
			std::shared_ptr<DbusTestTask> retval(DBUS_TEST_TASK(g_object_ref(mock)), [](DbusTestTask * task) { g_clear_object(&task); });
			return retval;
		}

		operator DbusTestTask* () {
			return DBUS_TEST_TASK(mock);
		}

		operator DbusTestDbusMock* () {
			return mock;
		}
};
