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
#include <functional>
#include <future>

#include <gtest/gtest.h>
#include <gio/gio.h>
#include <libdbustest/dbus-test.h>

class IndicatorFixture : public ::testing::Test
{
	private:
		std::string _indicatorPath;
		std::string _indicatorAddress;
		std::vector<std::shared_ptr<DbusTestTask>> _mocks;
	protected:
		std::chrono::milliseconds _eventuallyTime;

	private:
		class PerRunData {
		public:
			/* We're private in the fixture but other than that we don't care,
			   we don't leak out. This object's purpose isn't to hide data it is
			   to make the lifecycle of the items more clear. */
			std::shared_ptr<GMenuModel>  _menu;
			std::shared_ptr<GActionGroup> _actions;
			DbusTestService * _session_service;
			DbusTestService * _system_service;
			DbusTestTask * _test_indicator;
			DbusTestTask * _test_dummy;
			GDBusConnection * _session;
			GDBusConnection * _system;

			PerRunData (const std::string& indicatorPath, const std::string& indicatorAddress, std::vector<std::shared_ptr<DbusTestTask>>& mocks)
				: _menu(nullptr)
				, _session(nullptr)
			{
				_session_service = dbus_test_service_new(nullptr);
				dbus_test_service_set_bus(_session_service, DBUS_TEST_SERVICE_BUS_SESSION);

				_system_service = dbus_test_service_new(nullptr);
				dbus_test_service_set_bus(_system_service, DBUS_TEST_SERVICE_BUS_SYSTEM);

				_test_indicator = DBUS_TEST_TASK(dbus_test_process_new(indicatorPath.c_str()));
				dbus_test_task_set_name(_test_indicator, "Indicator");
				dbus_test_service_add_task(_session_service, _test_indicator);

				_test_dummy = dbus_test_task_new();
				dbus_test_task_set_wait_for(_test_dummy, indicatorAddress.c_str());
				dbus_test_task_set_name(_test_dummy, "Dummy");
				dbus_test_service_add_task(_session_service, _test_dummy);

				std::for_each(mocks.begin(), mocks.end(), [this](std::shared_ptr<DbusTestTask> task) {
					if (dbus_test_task_get_bus(task.get()) == DBUS_TEST_SERVICE_BUS_SYSTEM) {
						dbus_test_service_add_task(_system_service, task.get());
					} else {
						dbus_test_service_add_task(_session_service, task.get());
					}
				});

				g_debug("Starting System Bus");
				dbus_test_service_start_tasks(_system_service);
				_system = g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, nullptr);
				g_dbus_connection_set_exit_on_close(_system, FALSE);

				g_debug("Starting Session Bus");
				dbus_test_service_start_tasks(_session_service);
				_session = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, nullptr);
				g_dbus_connection_set_exit_on_close(_session, FALSE);
			}

			virtual ~PerRunData (void) {
				_menu.reset();
				_actions.reset();

				/* D-Bus Test Stuff */
				g_clear_object(&_test_dummy);
				g_clear_object(&_test_indicator);
				g_clear_object(&_session_service);
				g_clear_object(&_system_service);

				/* Wait for D-Bus session bus to go */
				if (!g_dbus_connection_is_closed(_session)) {
					g_dbus_connection_close_sync(_session, nullptr, nullptr);
				}
				g_clear_object(&_session);

				if (!g_dbus_connection_is_closed(_system)) {
					g_dbus_connection_close_sync(_system, nullptr, nullptr);
				}
				g_clear_object(&_system);
			}
		};

		std::shared_ptr<PerRunData> run;

	public:
		virtual ~IndicatorFixture() = default;

		IndicatorFixture (const std::string& path,
				const std::string& addr)
			: _indicatorPath(path)
			, _indicatorAddress(addr)
			, _eventuallyTime(std::chrono::seconds(5))
		{
		};


	protected:
		virtual void SetUp() override
		{
			run = std::make_shared<PerRunData>(_indicatorPath, _indicatorAddress, _mocks);

			_mocks.clear();
		}

		virtual void TearDown() override
		{
			run.reset();
		}

		void addMock (std::shared_ptr<DbusTestTask> mock)
		{
			_mocks.push_back(mock);
		}

		std::shared_ptr<DbusTestTask> buildBustleMock (const std::string& filename, DbusTestServiceBus bus = DBUS_TEST_SERVICE_BUS_BOTH)
		{
			return std::shared_ptr<DbusTestTask>([filename, bus]() {
				DbusTestTask * bustle = DBUS_TEST_TASK(dbus_test_bustle_new(filename.c_str()));
				dbus_test_task_set_name(bustle, "Bustle");
				dbus_test_task_set_bus(bustle, bus);
				return bustle;
			}(), [](DbusTestTask * bustle) {
				g_clear_object(&bustle);
			});
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

		testing::AssertionResult expectEventually (std::function<testing::AssertionResult(void)> &testfunc) {
			auto loop = std::shared_ptr<GMainLoop>(g_main_loop_new(nullptr, FALSE), [](GMainLoop * loop) { if (loop != nullptr) g_main_loop_unref(loop); });

			std::promise<testing::AssertionResult> retpromise;
			auto retfuture = retpromise.get_future();
			auto start = std::chrono::steady_clock::now();

			/* The core of the idle function as an object so we can use the C++-isms
			   of attaching the variables and make this code reasonably readable */
			std::function<void(void)> idlefunc = [&loop, &retpromise, &testfunc, &start, this]() -> void {
				auto result = testfunc();

				if (result == false && _eventuallyTime > (std::chrono::steady_clock::now() - start)) {
					return;
				}

				retpromise.set_value(result);
				g_main_loop_quit(loop.get());
			};

			auto idlesrc = g_idle_add([](gpointer data) -> gboolean {
				auto func = reinterpret_cast<std::function<void(void)> *>(data);
				(*func)();
				return G_SOURCE_CONTINUE;
			}, &idlefunc);

			g_main_loop_run(loop.get());
			g_source_remove(idlesrc);

			return retfuture.get();
		}

	protected:
		void setMenu (const std::string& path) {
			run->_menu.reset();

			g_debug("Getting Menu: %s:%s", _indicatorAddress.c_str(), path.c_str());
			run->_menu = std::shared_ptr<GMenuModel>(G_MENU_MODEL(g_dbus_menu_model_get(run->_session, _indicatorAddress.c_str(), path.c_str())), [](GMenuModel * modelptr) {
				g_clear_object(&modelptr);
			});

			menuWaitForItems(run->_menu);
		}

		void setActions (const std::string& path) {
			run->_actions.reset();

			run->_actions = std::shared_ptr<GActionGroup>(G_ACTION_GROUP(g_dbus_action_group_get(run->_session, _indicatorAddress.c_str(), path.c_str())), [](GActionGroup * groupptr) {
				g_clear_object(&groupptr);
			});

			agWaitForActions(run->_actions);
		}

		testing::AssertionResult expectActionExists (const gchar * nameStr, const std::string& name) {
			bool hasit = g_action_group_has_action(run->_actions.get(), name.c_str());

			if (!hasit) {
				auto result = testing::AssertionFailure();
				result <<
					"    Action: " << nameStr << std::endl <<
					"  Expected: " << "Exists" << std::endl <<
					"    Actual: " << "No action found" << std::endl;

				return result;
			}

			auto result = testing::AssertionSuccess();
			return result;
		}

		template <typename... Args> testing::AssertionResult expectEventuallyActionStateExists (Args&& ... args) {
			std::function<testing::AssertionResult(void)> func = [&]() {
				return expectActionStateExists(std::forward<Args>(args)...);
			};
			return expectEventually(func);
		}

		testing::AssertionResult expectActionStateType (const char * nameStr, const char * typeStr, const std::string& name, const GVariantType * type) {
			auto atype = g_action_group_get_action_state_type(run->_actions.get(), name.c_str());
			bool same = false;

			if (atype != nullptr) {
				same = g_variant_type_equal(atype, type);
			}

			if (!same) {
				auto result = testing::AssertionFailure();
				result <<
					"    Action: " << nameStr << std::endl <<
					"  Expected: " << typeStr << std::endl <<
					"    Actual: " << g_variant_type_peek_string(atype) << std::endl;

				return result;
			}

			auto result = testing::AssertionSuccess();
			return result;
		}

		template <typename... Args> testing::AssertionResult expectEventuallyActionStateType (Args&& ... args) {
			std::function<testing::AssertionResult(void)> func = [&]() {
				return expectActionStateType(std::forward<Args>(args)...);
			};
			return expectEventually(func);
		}

		testing::AssertionResult expectActionStateIs (const char * nameStr, const char * valueStr, const std::string& name, GVariant * value) {
			auto varref = std::shared_ptr<GVariant>(g_variant_ref_sink(value), [](GVariant * varptr) {
				if (varptr != nullptr)
					g_variant_unref(varptr);
			});
			auto aval = std::shared_ptr<GVariant>(g_action_group_get_action_state(run->_actions.get(), name.c_str()), [] (GVariant * varptr) {
				if (varptr != nullptr)
					g_variant_unref(varptr);
			});
			bool match = false;

			if (aval != nullptr) {
				match = g_variant_equal(aval.get(), varref.get());
			}

			if (!match) {
				gchar * attstr = nullptr;

				if (aval != nullptr) {
					attstr = g_variant_print(aval.get(), TRUE);
				} else {
					attstr = g_strdup("nullptr");
				}

				auto result = testing::AssertionFailure();
				result <<
					"    Action: " << nameStr << std::endl <<
					"  Expected: " << valueStr << std::endl <<
					"    Actual: " << attstr << std::endl;

				g_free(attstr);

				return result;
			} else {
				auto result = testing::AssertionSuccess();
				return result;
			}
		}

		testing::AssertionResult expectActionStateIs (const char * nameStr, const char * valueStr, const std::string& name, bool value) {
			GVariant * var = g_variant_new_boolean(value);
			return expectActionStateIs(nameStr, valueStr, name, var);
		}

		testing::AssertionResult expectActionStateIs (const char * nameStr, const char * valueStr, const std::string& name, std::string value) {
			GVariant * var = g_variant_new_string(value.c_str());
			return expectActionStateIs(nameStr, valueStr, name, var);
		}

		testing::AssertionResult expectActionStateIs (const char * nameStr, const char * valueStr, const std::string& name, const char * value) {
			GVariant * var = g_variant_new_string(value);
			return expectActionStateIs(nameStr, valueStr, name, var);
		}

		testing::AssertionResult expectActionStateIs (const char * nameStr, const char * valueStr, const std::string& name, double value) {
			GVariant * var = g_variant_new_double(value);
			return expectActionStateIs(nameStr, valueStr, name, var);
		}

		testing::AssertionResult expectActionStateIs (const char * nameStr, const char * valueStr, const std::string& name, float value) {
			GVariant * var = g_variant_new_double(value);
			return expectActionStateIs(nameStr, valueStr, name, var);
		}

		template <typename... Args> testing::AssertionResult expectEventuallyActionStateIs (Args&& ... args) {
			std::function<testing::AssertionResult(void)> func = [&]() {
				return expectActionStateIs(std::forward<Args>(args)...);
			};
			return expectEventually(func);
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
		testing::AssertionResult expectMenuAttribute (const char * menuLocationStr, const char * attributeStr, const char * valueStr, const std::vector<int> menuLocation, const std::string& attribute, GVariant * value) {
			auto varref = std::shared_ptr<GVariant>(g_variant_ref_sink(value), [](GVariant * varptr) {
				if (varptr != nullptr)
					g_variant_unref(varptr);
			});

			auto attrib = getMenuAttributeRecurse(menuLocation.cbegin(), menuLocation.cend(), attribute, varref, run->_menu);
			bool same = false;

			if (attrib != nullptr && varref != nullptr) {
				same = g_variant_equal(attrib.get(), varref.get());
			}

			if (!same) {
				gchar * attstr = nullptr;

				if (attrib != nullptr) {
					attstr = g_variant_print(attrib.get(), TRUE);
				} else {
					attstr = g_strdup("nullptr");
				}

				auto result = testing::AssertionFailure();
				result <<
					"      Menu: " << menuLocationStr << std::endl <<
					" Attribute: " << attributeStr << std::endl <<
					"  Expected: " << valueStr << std::endl <<
					"    Actual: " << attstr << std::endl;

				g_free(attstr);

				return result;
			} else {
				auto result = testing::AssertionSuccess();
				return result;
			}
		}

		testing::AssertionResult expectMenuAttribute (const char * menuLocationStr, const char * attributeStr, const char * valueStr, const std::vector<int> menuLocation, const std::string& attribute, bool value) {
			GVariant * var = g_variant_new_boolean(value);
			return expectMenuAttribute(menuLocationStr, attributeStr, valueStr, menuLocation, attribute, var);
		}

		testing::AssertionResult expectMenuAttribute (const char * menuLocationStr, const char * attributeStr, const char * valueStr, const std::vector<int> menuLocation, const std::string& attribute, std::string value) {
			GVariant * var = g_variant_new_string(value.c_str());
			return expectMenuAttribute(menuLocationStr, attributeStr, valueStr, menuLocation, attribute, var);
		}

		testing::AssertionResult expectMenuAttribute (const char * menuLocationStr, const char * attributeStr, const char * valueStr, const std::vector<int> menuLocation, const std::string& attribute, const char * value) {
			GVariant * var = g_variant_new_string(value);
			return expectMenuAttribute(menuLocationStr, attributeStr, valueStr, menuLocation, attribute, var);
		}

		template <typename... Args> testing::AssertionResult expectEventuallyMenuAttribute (Args&& ... args) {
			std::function<testing::AssertionResult(void)> func = [&]() {
				return expectMenuAttribute(std::forward<Args>(args)...);
			};
			return expectEventually(func);
		}
};

/* Menu Attrib */
#define ASSERT_MENU_ATTRIB(menu, attrib, value) \
	ASSERT_PRED_FORMAT3(IndicatorFixture::expectMenuAttribute, menu, attrib, value)

#define EXPECT_MENU_ATTRIB(menu, attrib, value) \
	EXPECT_PRED_FORMAT3(IndicatorFixture::expectMenuAttribute, menu, attrib, value)

#define EXPECT_EVENTUALLY_MENU_ATTRIB(menu, attrib, value) \
	EXPECT_PRED_FORMAT3(IndicatorFixture::expectEventuallyMenuAttribute, menu, attrib, value)

/* Action Exists */
#define ASSERT_ACTION_EXISTS(action) \
	ASSERT_PRED_FORMAT1(IndicatorFixture::expectActionExists, action)

#define EXPECT_ACTION_EXISTS(action) \
	EXPECT_PRED_FORMAT1(IndicatorFixture::expectActionExists, action)

#define EXPECT_EVENTUALLY_ACTION_EXISTS(action) \
	EXPECT_PRED_FORMAT1(IndicatorFixture::expectEventuallyActionExists, action)

/* Action State */
#define ASSERT_ACTION_STATE(action, value) \
	ASSERT_PRED_FORMAT2(IndicatorFixture::expectActionStateIs, action, value)

#define EXPECT_ACTION_STATE(action, value) \
	EXPECT_PRED_FORMAT2(IndicatorFixture::expectActionStateIs, action, value)

#define EXPECT_EVENTUALLY_ACTION_STATE(action, value) \
	EXPECT_PRED_FORMAT2(IndicatorFixture::expectEventuallyActionStateIs, action, value)

/* Action State Type */
#define ASSERT_ACTION_STATE_TYPE(action, type) \
	ASSERT_PRED_FORMAT2(IndicatorFixture::expectActionStateType, action, type)

#define EXPECT_ACTION_STATE_TYPE(action, type) \
	EXPECT_PRED_FORMAT2(IndicatorFixture::expectActionStateType, action, type)

#define EXPECT_EVENTUALLY_ACTION_STATE_TYPE(action, type) \
	EXPECT_PRED_FORMAT2(IndicatorFixture::expectEventuallyActionStateType, action, type)
