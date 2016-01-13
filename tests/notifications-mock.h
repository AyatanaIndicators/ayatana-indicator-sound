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

#include <algorithm>
#include <map>
#include <memory>
#include <type_traits>

#include <libdbustest/dbus-test.h>

class NotificationsMock
{
		DbusTestDbusMock * mock = nullptr;
		DbusTestDbusMockObject * baseobj = nullptr;

	public:
		NotificationsMock (std::vector<std::string> capabilities = {"actions", "body", "body-markup", "icon-static", "image/svg+xml", "x-canonical-private-synchronous", "x-canonical-append", "x-canonical-private-icon-only", "x-canonical-truncation", "private-synchronous", "append", "private-icon-only", "truncation"}) {
			mock = dbus_test_dbus_mock_new("org.freedesktop.Notifications");
			dbus_test_task_set_bus(DBUS_TEST_TASK(mock), DBUS_TEST_SERVICE_BUS_SESSION);
			dbus_test_task_set_name(DBUS_TEST_TASK(mock), "Notify");

			baseobj =dbus_test_dbus_mock_get_object(mock, "/org/freedesktop/Notifications", "org.freedesktop.Notifications", nullptr);

			std::string capspython("ret = ");
			capspython += vector2py(capabilities);
			dbus_test_dbus_mock_object_add_method(mock, baseobj,
				"GetCapabilities", nullptr, G_VARIANT_TYPE("as"),
				capspython.c_str(), nullptr);

			dbus_test_dbus_mock_object_add_method(mock, baseobj,
				"GetServerInformation", nullptr, G_VARIANT_TYPE("(ssss)"),
				"ret = ['notification-mock', 'Testing harness', '1.0', '1.1']", nullptr);

			dbus_test_dbus_mock_object_add_method(mock, baseobj,
				"Notify", G_VARIANT_TYPE("(susssasa{sv}i)"), G_VARIANT_TYPE("u"),
				"ret = 10", nullptr);

			dbus_test_dbus_mock_object_add_method(mock, baseobj,
				"CloseNotification", G_VARIANT_TYPE("u"), nullptr,
				"", nullptr);
		}

		~NotificationsMock () {
			g_debug("Destroying the Notifications Mock");
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

		struct Notification {
			std::string app_name;
			unsigned int replace_id;
			std::string app_icon;
			std::string summary;
			std::string body;
			std::vector<std::string> actions;
			std::map<std::string, std::shared_ptr<GVariant>> hints;
			int timeout;
		};

		std::shared_ptr<GVariant> childGet (GVariant * tuple, gsize index) {
			return std::shared_ptr<GVariant>(g_variant_get_child_value(tuple, index),
				[](GVariant * v){ if (v != nullptr) g_variant_unref(v); });
		}

		std::vector<Notification> getNotifications (void) {
			std::vector<Notification> notifications;

			unsigned int cnt, i;
			auto calls = dbus_test_dbus_mock_object_get_method_calls(mock, baseobj, "Notify", &cnt, nullptr);

			for (i = 0; i < cnt; i++) {
				auto call = calls[i];
				Notification notification;

				notification.app_name = g_variant_get_string(childGet(call.params, 0).get(), nullptr);
				notification.replace_id = g_variant_get_uint32(childGet(call.params, 1).get());
				notification.app_icon = g_variant_get_string(childGet(call.params, 2).get(), nullptr);
				notification.summary = g_variant_get_string(childGet(call.params, 3).get(), nullptr);
				notification.body = g_variant_get_string(childGet(call.params, 4).get(), nullptr);
				notification.timeout = g_variant_get_int32(childGet(call.params, 7).get());

				auto vactions = childGet(call.params, 5);
				GVariantIter iactions = {0};
				g_variant_iter_init(&iactions, vactions.get());
				const gchar * action = nullptr;
				while (g_variant_iter_loop(&iactions, "&s", &action)) {
					std::string saction(action);
					notification.actions.push_back(saction);
				}

				auto vhints = childGet(call.params, 6);
				GVariantIter ihints = {0};
				g_variant_iter_init(&ihints, vhints.get());
				const gchar * hint_key = nullptr;
				GVariant * hint_value = nullptr;
				while (g_variant_iter_loop(&ihints, "{&sv}", &hint_key, &hint_value)) {
					std::string key(hint_key);
					std::shared_ptr<GVariant> value(g_variant_ref(hint_value), [](GVariant * v){ if (v != nullptr) g_variant_unref(v); });
					notification.hints[key] = value;
				}

				notifications.push_back(notification);
			}

			return notifications;
		}

		bool clearNotifications (void) {
			return dbus_test_dbus_mock_object_clear_method_calls(mock, baseobj, nullptr);
		}
};
