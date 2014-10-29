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

class IndicatorFixture : public ::testing::Test
{
	private:
		std::string _indicatorPath;
		std::string _indicatorAddress;
		GMainLoop * _loop;
		GMenuModel * _menu;
		DbusTestService * _test_service;
		DbusTestTask * _test_indicator;
		DbusTestTask * _test_dummy;
		GDBusConnection * _session;

	public:
		virtual ~IndicatorFixture() = default;

		IndicatorFixture (const std::string& path,
				const std::string& addr)
			: _indicatorPath(path)
			, _indicatorAddress(addr)
			, _loop(nullptr)
			, _menu(nullptr)
			, _session(nullptr)
		{
		};


	protected:
		virtual void SetUp() override
		{
			_loop = g_main_loop_new(nullptr, FALSE);

			_test_service = dbus_test_service_new(nullptr);

			_test_indicator = DBUS_TEST_TASK(dbus_test_process_new(_indicatorPath.c_str()));
			dbus_test_service_add_task(_test_service, _test_indicator);

			_test_dummy = dbus_test_task_new();
			dbus_test_task_set_wait_for(_test_dummy, _indicatorAddress.c_str());
			dbus_test_service_add_task(_test_service, _test_dummy);

			DbusTestBustle * bustle = dbus_test_bustle_new("indicator-test.bustle");
			dbus_test_service_add_task(_test_service, DBUS_TEST_TASK(bustle));
			g_object_unref(bustle);

			dbus_test_service_start_tasks(_test_service);

			_session = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, nullptr);
		}

		virtual void TearDown() override
		{
			/* Menu structures that could be allocated */
			g_clear_object(&_menu);

			/* D-Bus Test Stuff */
			g_clear_object(&_test_dummy);
			g_clear_object(&_test_indicator);
			g_clear_object(&_test_service);

			/* Wait for D-Bus session bus to go */
			if (!g_dbus_connection_is_closed(_session)) {
				g_dbus_connection_close_sync(_session, nullptr, nullptr);
			}
			g_clear_object(&_session);

			/* Dropping temp loop */
			g_main_loop_unref(_loop);
		}

		static void _changed_quit (GMenuModel * model, gint position, gint removed, gint added, GMainLoop * loop) {
			g_debug("Got Menus");
			g_main_loop_quit(loop);
		}

		static gboolean _loop_quit (gpointer user_data) {
			g_warning("Menu Timeout");
			g_main_loop_quit((GMainLoop *)user_data);
			return G_SOURCE_CONTINUE;
		}

		void setMenu (const std::string& path) {
			g_clear_object(&_menu);
			g_debug("Getting Menu: %s:%s", _indicatorAddress.c_str(), path.c_str());
			_menu = G_MENU_MODEL(g_dbus_menu_model_get(_session, _indicatorAddress.c_str(), path.c_str()));

			/* Our two exit criteria */
			gulong signal = g_signal_connect(G_OBJECT(_menu), "items-changed", G_CALLBACK(_changed_quit), _loop);
			guint timer = g_timeout_add_seconds(5, _loop_quit, _loop);

			g_menu_model_get_n_items(_menu);

			/* Wait for sync */
			g_main_loop_run(_loop);

			/* Clean up */
			g_source_remove(timer);
			g_signal_handler_disconnect(G_OBJECT(_menu), signal);
		}

		void expectActionExists (const std::string& name) {

		}

		void expectActionStateType (const std::string& name, const GVariantType * type) {

		}

		void expectActionStateIs (const std::string& name, const GVariant * value) {

		}

		void expectMenuAttributeVerify (int location, GMenuModel * menu, const std::string& attribute, GVariant * value) {
			EXPECT_LT(location, g_menu_model_get_n_items(menu));
			if (location >= g_menu_model_get_n_items(menu))
				return;

			auto menuval = g_menu_model_get_item_attribute_value(menu, location, attribute.c_str(), g_variant_get_type(value));
			EXPECT_NE(nullptr, menuval);
			if (menuval != nullptr) {
				EXPECT_TRUE(g_variant_equal(value, menuval));
				g_variant_unref(menuval);
			}
		}

		void expectMenuAttributeRecurse (const std::vector<int> menuLocation, const std::string& attribute, GVariant * value, unsigned int index, GMenuModel * menu) {
			ASSERT_LT(index, menuLocation.size());

			if (menuLocation.size() - 1 == index)
				return expectMenuAttributeVerify(menuLocation[index], menu, attribute, value);

			auto submenu = g_menu_model_get_item_link(menu, menuLocation[index], G_MENU_LINK_SUBMENU);
			EXPECT_NE(nullptr, submenu);
			if (submenu == nullptr)
				return;

			expectMenuAttributeRecurse(menuLocation, attribute, value, index++, submenu);
			g_object_unref(submenu);
		}

		void expectMenuAttribute (const std::vector<int> menuLocation, const std::string& attribute, GVariant * value) {
			g_variant_ref_sink(value);
			expectMenuAttributeRecurse(menuLocation, attribute, value, 0, _menu);
			g_variant_unref(value);
		}

		void expectMenuAttribute (const std::vector<int> menuLocation, const std::string& attribute, bool value) {
			GVariant * var = g_variant_new_boolean(value);
			expectMenuAttribute(menuLocation, attribute, var);
		}

		void expectMenuAttribute (const std::vector<int> menuLocation, const std::string& attribute, std::string value) {
			GVariant * var = g_variant_new_string(value.c_str());
			expectMenuAttribute(menuLocation, attribute, var);
		}

		void expectMenuAttribute (const std::vector<int> menuLocation, const std::string& attribute, const char * value) {
			GVariant * var = g_variant_new_string(value);
			expectMenuAttribute(menuLocation, attribute, var);
		}

};

#define EXPECT_MENU_ATTRIB(menu, attrib, value) expectMenuAttribute(menu, attrib, value)

