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

#include <memory>

#include <gtest/gtest.h>
#include <gio/gio.h>
#include <libdbustest/dbus-test.h>

#include "notifications-mock.h"

extern "C" {
#include "indicator-sound-service.h"
#include "vala-mocks.h"
}

class NotificationsTest : public ::testing::Test
{
	protected:
		DbusTestService * service = NULL;
		DbusTestDbusMock * mock = NULL;

		GDBusConnection * session = NULL;
		std::shared_ptr<NotificationsMock> notifications;

		virtual void SetUp() {
			service = dbus_test_service_new(NULL);
			dbus_test_service_set_bus(service, DBUS_TEST_SERVICE_BUS_SESSION);

			notifications = std::make_shared<NotificationsMock>();

			dbus_test_service_add_task(service, (DbusTestTask*)*notifications);
			dbus_test_service_start_tasks(service);

			session = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
			ASSERT_NE(nullptr, session);
			g_dbus_connection_set_exit_on_close(session, FALSE);
			g_object_add_weak_pointer(G_OBJECT(session), (gpointer *)&session);
		}

		virtual void TearDown() {
			g_clear_object(&mock);
			g_clear_object(&service);

			g_object_unref(session);

			unsigned int cleartry = 0;
			while (session != NULL && cleartry < 100) {
				loop(100);
				cleartry++;
			}

			ASSERT_EQ(nullptr, session);
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

		static int unref_idle (gpointer user_data) {
			g_variant_unref(static_cast<GVariant *>(user_data));
			return G_SOURCE_REMOVE;
		}
};

TEST_F(NotificationsTest, BasicObject) {
	auto playerList = std::shared_ptr<MediaPlayerList>(MEDIA_PLAYER_LIST(media_player_list_mock_new()), [](MediaPlayerList * list){g_clear_object(&list);});
	auto volumeControl = std::shared_ptr<VolumeControl>(VOLUME_CONTROL(volume_control_mock_new()), [](VolumeControl * control){g_clear_object(&control);});
	auto soundService = std::shared_ptr<IndicatorSoundService>(indicator_sound_service_new(playerList.get(), volumeControl.get()), [](IndicatorSoundService * service){g_clear_object(&service);});

	/* Give some time settle */
	loop(50);
}
