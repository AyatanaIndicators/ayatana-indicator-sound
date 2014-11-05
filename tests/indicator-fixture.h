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

#include <memory>
#include <algorithm>
#include <string>

#include <gtest/gtest.h>
#include <gio/gio.h>
#include <libdbustest/dbus-test.h>

extern "C" {

/* Super useful function that GLib has hidden from us :-( */
gboolean
g_dbus_action_group_sync (GDBusActionGroup  *group,
                          GCancellable      *cancellable,
                          GError           **error);

}

class IndicatorFixture : public ::testing::Test
{
	private:
		std::string _indicatorPath;
		std::string _indicatorAddress;
		std::shared_ptr<GMenuModel>  _menu;
		std::shared_ptr<GActionGroup> _actions;
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
			, _menu(nullptr)
			, _session(nullptr)
		{
		};


	protected:
		virtual void SetUp() override
		{

			_test_service = dbus_test_service_new(nullptr);

			_test_indicator = DBUS_TEST_TASK(dbus_test_process_new(_indicatorPath.c_str()));
			dbus_test_task_set_name(_test_indicator, "Indicator");
			dbus_test_service_add_task(_test_service, _test_indicator);

			_test_dummy = dbus_test_task_new();
			dbus_test_task_set_wait_for(_test_dummy, _indicatorAddress.c_str());
			dbus_test_task_set_name(_test_dummy, "Dummy");
			dbus_test_service_add_task(_test_service, _test_dummy);

			if (true) {
				DbusTestBustle * bustle = dbus_test_bustle_new("indicator-test.bustle");
				dbus_test_task_set_name(DBUS_TEST_TASK(bustle), "Bustle");
				dbus_test_service_add_task(_test_service, DBUS_TEST_TASK(bustle));
				g_object_unref(bustle);
			}

			dbus_test_service_start_tasks(_test_service);

			_session = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, nullptr);
		}

		virtual void TearDown() override
		{
			_menu.reset();
			_actions.reset();

			/* D-Bus Test Stuff */
			g_clear_object(&_test_dummy);
			g_clear_object(&_test_indicator);
			g_clear_object(&_test_service);

			/* Wait for D-Bus session bus to go */
			if (!g_dbus_connection_is_closed(_session)) {
				g_dbus_connection_close_sync(_session, nullptr, nullptr);
			}
			g_clear_object(&_session);
		}

	private:
		void waitForCore (GObject * obj, const gchar * signalname) {
			auto loop = g_main_loop_new(nullptr, FALSE);

			/* Our two exit criteria */
			gulong signal = g_signal_connect_swapped(obj, signalname, G_CALLBACK(g_main_loop_quit), loop);
			guint timer = g_timeout_add_seconds(5, [](gpointer user_data) -> gboolean {
					g_warning("Menu Timeout");
					g_main_loop_quit((GMainLoop *)user_data);
					return G_SOURCE_CONTINUE;
				}, loop);

			/* Wait for sync */
			g_main_loop_run(loop);

			/* Clean up */
			g_source_remove(timer);
			g_signal_handler_disconnect(obj, signal);

			g_main_loop_unref(loop);
		}

		void menuWaitForItems (const std::shared_ptr<GMenuModel>& menu) {
			auto count = g_menu_model_get_n_items(menu.get());
			
			if (count != 0)
				return;

			waitForCore(G_OBJECT(menu.get()), "items-changed");
		}

		void agWaitForActions (const std::shared_ptr<GActionGroup>& group) {
			auto list = std::shared_ptr<gchar *>(
				g_action_group_list_actions(group.get()),
				[](gchar ** list) {
					g_strfreev(list);
				});

			if (g_strv_length(list.get()) != 0) {
				return;
			}

			waitForCore(G_OBJECT(group.get()), "action-added");
		}

	protected:
		void setMenu (const std::string& path) {
			_menu.reset();

			g_debug("Getting Menu: %s:%s", _indicatorAddress.c_str(), path.c_str());
			_menu = std::shared_ptr<GMenuModel>(G_MENU_MODEL(g_dbus_menu_model_get(_session, _indicatorAddress.c_str(), path.c_str())), [](GMenuModel * modelptr) {
				g_clear_object(&modelptr);
			});

			menuWaitForItems(_menu);
		}

		void setActions (const std::string& path) {
			_actions.reset();

			_actions = std::shared_ptr<GActionGroup>(G_ACTION_GROUP(g_dbus_action_group_get(_session, _indicatorAddress.c_str(), path.c_str())), [](GActionGroup * groupptr) {
				g_clear_object(&groupptr);
			});

			agWaitForActions(_actions);
		}

		bool expectActionExists (const std::string& name) {
			bool hasit = g_action_group_has_action(_actions.get(), name.c_str());

			if (!hasit) {
				std::cout <<
					"    Action: " << name << std::endl <<
					"  Expected: " << "Exists" << std::endl <<
					"    Actual: " << "No action found" << std::endl;
			}

			return hasit;
		}

		bool expectActionStateType (const std::string& name, const GVariantType * type) {
			auto atype = g_action_group_get_action_state_type(_actions.get(), name.c_str());
			bool same = false;

			if (atype != nullptr) {
				same = g_variant_type_equal(atype, type);
			}

			if (!same) {
				std::cout <<
					"    Action: " << name << std::endl <<
					"  Expected: " << g_variant_type_peek_string(type) << std::endl <<
					"    Actual: " << g_variant_type_peek_string(atype) << std::endl;
			}

			return same;
		}

		bool expectActionStateIs (const std::string& name, GVariant * value) {
			auto varref = std::shared_ptr<GVariant>(g_variant_ref_sink(value), [](GVariant * varptr) {
				if (varptr != nullptr)
					g_variant_unref(varptr);
			});
			auto aval = std::shared_ptr<GVariant>(g_action_group_get_action_state(_actions.get(), name.c_str()), [] (GVariant * varptr) {
				if (varptr != nullptr)
					g_variant_unref(varptr);
			});
			bool match = false;

			if (aval != nullptr) {
				match = g_variant_equal(aval.get(), varref.get());
			}

			if (!match) {
				gchar * valstr = nullptr;
				gchar * attstr = nullptr;

				if (aval != nullptr) {
					attstr = g_variant_print(aval.get(), TRUE);
				} else {
					attstr = g_strdup("nullptr");
				}

				if (varref != nullptr) {
					valstr = g_variant_print(varref.get(), TRUE);
				} else {
					valstr = g_strdup("nullptr");
				}

				std::cout <<
					"    Action: " << name << std::endl <<
					"  Expected: " << valstr << std::endl <<
					"    Actual: " << attstr << std::endl;

				g_free(valstr);
				g_free(attstr);
			}

			return match;
		}

		bool expectActionStateIs (const std::string& name, bool value) {
			GVariant * var = g_variant_new_boolean(value);
			return expectActionStateIs(name, var);
		}

		bool expectActionStateIs (const std::string& name, std::string value) {
			GVariant * var = g_variant_new_string(value.c_str());
			return expectActionStateIs(name, var);
		}

		bool expectActionStateIs (const std::string& name, const char * value) {
			GVariant * var = g_variant_new_string(value);
			return expectActionStateIs(name, var);
		}

		bool expectActionStateIs (const std::string& name, double value) {
			GVariant * var = g_variant_new_double(value);
			return expectActionStateIs(name, var);
		}

		bool expectActionStateIs (const std::string& name, float value) {
			GVariant * var = g_variant_new_double(value);
			return expectActionStateIs(name, var);
		}


	private:
		std::shared_ptr<GVariant> getMenuAttributeVal (int location, std::shared_ptr<GMenuModel>& menu, const std::string& attribute, std::shared_ptr<GVariant>& value) {
			if (!(location < g_menu_model_get_n_items(menu.get()))) {
				return nullptr;
			}

			if (location >= g_menu_model_get_n_items(menu.get()))
				return nullptr;

			auto menuval = std::shared_ptr<GVariant>(g_menu_model_get_item_attribute_value(menu.get(), location, attribute.c_str(), g_variant_get_type(value.get())), [](GVariant * varptr) {
				if (varptr != nullptr)
					g_variant_unref(varptr);
			});

			return menuval;
		}

		std::shared_ptr<GVariant> getMenuAttributeRecurse (std::vector<int>::const_iterator menuLocation, std::vector<int>::const_iterator menuEnd, const std::string& attribute, std::shared_ptr<GVariant>& value, std::shared_ptr<GMenuModel>& menu) {
			if (menuLocation == menuEnd)
				return nullptr;

			if (menuLocation + 1 == menuEnd)
				return getMenuAttributeVal(*menuLocation, menu, attribute, value);

			auto clearfunc = [](GMenuModel * modelptr) {
				g_clear_object(&modelptr);
			};

			auto submenu = std::shared_ptr<GMenuModel>(g_menu_model_get_item_link(menu.get(), *menuLocation, G_MENU_LINK_SUBMENU), clearfunc);

			if (submenu == nullptr)
				submenu = std::shared_ptr<GMenuModel>(g_menu_model_get_item_link(menu.get(), *menuLocation, G_MENU_LINK_SECTION), clearfunc);

			if (submenu == nullptr)
				return nullptr;

			menuWaitForItems(submenu);

			return getMenuAttributeRecurse(menuLocation + 1, menuEnd, attribute, value, submenu);
		}

	protected:
		bool expectMenuAttribute (const std::vector<int> menuLocation, const std::string& attribute, GVariant * value) {
			auto varref = std::shared_ptr<GVariant>(g_variant_ref_sink(value), [](GVariant * varptr) {
				if (varptr != nullptr)
					g_variant_unref(varptr);
			});

			auto attrib = getMenuAttributeRecurse(menuLocation.cbegin(), menuLocation.cend(), attribute, varref, _menu);
			bool same = false;

			if (attrib != nullptr && varref != nullptr) {
				same = g_variant_equal(attrib.get(), varref.get());
			}

			if (!same) {
				gchar * valstr = nullptr;
				gchar * attstr = nullptr;

				if (attrib != nullptr) {
					attstr = g_variant_print(attrib.get(), TRUE);
				} else {
					attstr = g_strdup("nullptr");
				}

				if (varref != nullptr) {
					valstr = g_variant_print(varref.get(), TRUE);
				} else {
					valstr = g_strdup("nullptr");
				}

				std::string menuprint("{ ");
				std::for_each(menuLocation.begin(), menuLocation.end(), [&menuprint](int i) {
					menuprint.append(std::to_string(i));
					menuprint.append(", ");
				});
				menuprint += "}";

				std::cout <<
					"      Menu: " << menuprint << std::endl <<
					" Attribute: " << attribute << std::endl <<
					"  Expected: " << valstr << std::endl <<
					"    Actual: " << attstr << std::endl;

				g_free(valstr);
				g_free(attstr);
			}

			return same;
		}

		bool expectMenuAttribute (const std::vector<int> menuLocation, const std::string& attribute, bool value) {
			GVariant * var = g_variant_new_boolean(value);
			return expectMenuAttribute(menuLocation, attribute, var);
		}

		bool expectMenuAttribute (const std::vector<int> menuLocation, const std::string& attribute, std::string value) {
			GVariant * var = g_variant_new_string(value.c_str());
			return expectMenuAttribute(menuLocation, attribute, var);
		}

		bool expectMenuAttribute (const std::vector<int> menuLocation, const std::string& attribute, const char * value) {
			GVariant * var = g_variant_new_string(value);
			return expectMenuAttribute(menuLocation, attribute, var);
		}

};

#define EXPECT_MENU_ATTRIB(menu, attrib, value) expectMenuAttribute(menu, attrib, value)
#define ASSERT_MENU_ATTRIB(menu, attrib, value) \
	if (!expectMenuAttribute(menu, attrib, value)) \
		return;

