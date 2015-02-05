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
		AccountsServiceMock () {
			mock = dbus_test_dbus_mock_new("org.freedesktop.Notifications");
			dbus_test_task_set_bus(DBUS_TEST_TASK(mock), DBUS_TEST_SERVICE_BUS_SESSION);
		}

		~AccountsServiceMock () {
			g_debug("Destroying the Accounts Service Mock");
			g_clear_object(&mock);
		}

		operator std::shared_ptr<DbusTestTask> () {
			return std::make_shared<DbusTestTask>(g_object_ref(mock), [](DbusTestTask * task) { g_clear_object(&task); });
		}

		operator DbusTestTask* () {
			return DBUS_TEST_TASK(mock);
		}

		operator DbusTestDbusMock* () {
			return mock;
		}

		DbusTestDbusMockObject * get_sound () {
			return soundobj;
		}
};
