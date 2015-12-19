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
#include <libnotify/notify.h>

#include "notifications-mock.h"
#include "gtest-gvariant.h"

extern "C" {
#include "indicator-sound-service.h"
#include "vala-mocks.h"
}

class NotificationsTest : public ::testing::Test
{
	protected:
		DbusTestService * service = NULL;

		GDBusConnection * session = NULL;
		std::shared_ptr<NotificationsMock> notifications;

		virtual void SetUp() {
			g_setenv("GSETTINGS_SCHEMA_DIR", SCHEMA_DIR, TRUE);
			g_setenv("GSETTINGS_BACKEND", "memory", TRUE);

			service = dbus_test_service_new(NULL);
			dbus_test_service_set_bus(service, DBUS_TEST_SERVICE_BUS_SESSION);

			/* Useful for debugging test failures, not needed all the time (until it fails) */
			#if 0
			auto bustle = std::shared_ptr<DbusTestTask>([]() {
				DbusTestTask * bustle = DBUS_TEST_TASK(dbus_test_bustle_new("notifications-test.bustle"));
				dbus_test_task_set_name(bustle, "Bustle");
				dbus_test_task_set_bus(bustle, DBUS_TEST_SERVICE_BUS_SESSION);
				return bustle;
			}(), [](DbusTestTask * bustle) {
				g_clear_object(&bustle);
			});
			dbus_test_service_add_task(service, bustle.get());
			#endif

			notifications = std::make_shared<NotificationsMock>();

			dbus_test_service_add_task(service, (DbusTestTask*)*notifications);
			dbus_test_service_start_tasks(service);

			session = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
			ASSERT_NE(nullptr, session);
			g_dbus_connection_set_exit_on_close(session, FALSE);
			g_object_add_weak_pointer(G_OBJECT(session), (gpointer *)&session);

			/* This is done in main.c */
			notify_init("indicator-sound");
		}

		virtual void TearDown() {
			if (notify_is_initted())
				notify_uninit();

			notifications.reset();
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

		std::shared_ptr<MediaPlayerList> playerListMock () {
			auto playerList = std::shared_ptr<MediaPlayerList>(
				MEDIA_PLAYER_LIST(media_player_list_mock_new()),
				[](MediaPlayerList * list) {
					g_clear_object(&list);
				});
			return playerList;
		}

		std::shared_ptr<IndicatorSoundOptions> optionsMock () {
			auto options = std::shared_ptr<IndicatorSoundOptions>(
				INDICATOR_SOUND_OPTIONS(options_mock_new()),
				[](IndicatorSoundOptions * options){
					g_clear_object(&options);
				});
			return options;
		}

		std::shared_ptr<VolumeControl> volumeControlMock (const std::shared_ptr<IndicatorSoundOptions>& optionsMock) {
			auto volumeControl = std::shared_ptr<VolumeControl>(
				VOLUME_CONTROL(volume_control_mock_new(optionsMock.get())),
				[](VolumeControl * control){
					g_clear_object(&control);
				});
			return volumeControl;
		}

		std::shared_ptr<IndicatorSoundService> standardService (std::shared_ptr<VolumeControl> volumeControl, std::shared_ptr<MediaPlayerList> playerList, const std::shared_ptr<IndicatorSoundOptions>& options) {
			auto soundService = std::shared_ptr<IndicatorSoundService>(
				indicator_sound_service_new(playerList.get(), volumeControl.get(), nullptr, options.get()),
				[](IndicatorSoundService * service){
					g_clear_object(&service);
				});

			return soundService;
		}

		void setMockVolume (std::shared_ptr<VolumeControl> volumeControl, double volume, VolumeControlVolumeReasons reason = VOLUME_CONTROL_VOLUME_REASONS_USER_KEYPRESS) {
			VolumeControlVolume * vol = volume_control_volume_new();
			vol->volume = volume;
			vol->reason = reason;

			volume_control_set_volume(volumeControl.get(), vol);
			g_object_unref(vol);
		}

		void setIndicatorShown (bool shown) {
			auto bus = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, nullptr);

			g_dbus_connection_call(bus,
				g_dbus_connection_get_unique_name(bus),
				"/com/canonical/indicator/sound",
				"org.gtk.Actions",
				"SetState",
				g_variant_new("(sva{sv})", "indicator-shown", g_variant_new_boolean(shown), nullptr),
				nullptr,
				G_DBUS_CALL_FLAGS_NONE,
				-1,
				nullptr,
				nullptr,
				nullptr);

			g_clear_object(&bus);
		}
};

TEST_F(NotificationsTest, BasicObject) {
	auto options = optionsMock();
	auto volumeControl = volumeControlMock(options);
	auto soundService = standardService(volumeControl, playerListMock(), options);

	/* Give some time settle */
	loop(50);

	/* Auto free */
}

TEST_F(NotificationsTest, VolumeChanges) {
	auto options = optionsMock();
	auto volumeControl = volumeControlMock(options);
	auto soundService = standardService(volumeControl, playerListMock(), options);

	/* Set a volume */
	notifications->clearNotifications();
	setMockVolume(volumeControl, 0.50);
	loop(50);
	auto notev = notifications->getNotifications();
	ASSERT_EQ(1, notev.size());
	EXPECT_EQ("indicator-sound", notev[0].app_name);
	EXPECT_EQ("Volume", notev[0].summary);
	EXPECT_EQ(0, notev[0].actions.size());
	EXPECT_GVARIANT_EQ("@s 'true'", notev[0].hints["x-canonical-private-synchronous"]);
	EXPECT_GVARIANT_EQ("@i 50", notev[0].hints["value"]);

	/* Set a different volume */
	notifications->clearNotifications();
	setMockVolume(volumeControl, 0.60);
	loop(50);
	notev = notifications->getNotifications();
	ASSERT_EQ(1, notev.size());
	EXPECT_GVARIANT_EQ("@i 60", notev[0].hints["value"]);

	/* Have pulse set a volume */
	notifications->clearNotifications();
	setMockVolume(volumeControl, 0.70, VOLUME_CONTROL_VOLUME_REASONS_PULSE_CHANGE);
	loop(50);
	notev = notifications->getNotifications();
	ASSERT_EQ(0, notev.size());

	/* Have AS set the volume */
	notifications->clearNotifications();
	setMockVolume(volumeControl, 0.80, VOLUME_CONTROL_VOLUME_REASONS_ACCOUNTS_SERVICE_SET);
	loop(50);
	notev = notifications->getNotifications();
	ASSERT_EQ(0, notev.size());
}

TEST_F(NotificationsTest, StreamChanges) {
	auto options = optionsMock();
	auto volumeControl = volumeControlMock(options);
	auto soundService = standardService(volumeControl, playerListMock(), options);

	/* Set a volume */
	notifications->clearNotifications();
	setMockVolume(volumeControl, 0.5);
	loop(50);
	auto notev = notifications->getNotifications();
	ASSERT_EQ(1, notev.size());

	/* Change Streams, no volume change */
	notifications->clearNotifications();
	volume_control_mock_set_mock_stream(VOLUME_CONTROL_MOCK(volumeControl.get()), "alarm");
	setMockVolume(volumeControl, 0.5, VOLUME_CONTROL_VOLUME_REASONS_VOLUME_STREAM_CHANGE);
	loop(50);
	notev = notifications->getNotifications();
	EXPECT_EQ(0, notev.size());

	/* Change Streams, volume change */
	notifications->clearNotifications();
	volume_control_mock_set_mock_stream(VOLUME_CONTROL_MOCK(volumeControl.get()), "alert");
	setMockVolume(volumeControl, 0.6, VOLUME_CONTROL_VOLUME_REASONS_VOLUME_STREAM_CHANGE);
	loop(50);
	notev = notifications->getNotifications();
	EXPECT_EQ(0, notev.size());

	/* Change Streams, no volume change, volume up */
	notifications->clearNotifications();
	volume_control_mock_set_mock_stream(VOLUME_CONTROL_MOCK(volumeControl.get()), "multimedia");
	setMockVolume(volumeControl, 0.6, VOLUME_CONTROL_VOLUME_REASONS_VOLUME_STREAM_CHANGE);
	loop(50);
	setMockVolume(volumeControl, 0.65);
	notev = notifications->getNotifications();
	EXPECT_EQ(1, notev.size());
	EXPECT_GVARIANT_EQ("@i 65", notev[0].hints["value"]);
}

TEST_F(NotificationsTest, IconTesting) {
	auto options = optionsMock();
	auto volumeControl = volumeControlMock(options);
	auto soundService = standardService(volumeControl, playerListMock(), options);

	/* Set an initial volume */
	notifications->clearNotifications();
	setMockVolume(volumeControl, 0.5);
	loop(50);
	auto notev = notifications->getNotifications();
	ASSERT_EQ(1, notev.size());

	/* Generate a set of notifications */
	notifications->clearNotifications();
	for (float i = 0.0; i < 1.01; i += 0.1) {
		setMockVolume(volumeControl, i);
	}

	loop(50);
	notev = notifications->getNotifications();
	ASSERT_EQ(11, notev.size());

	EXPECT_EQ("audio-volume-muted",  notev[0].app_icon);
	EXPECT_EQ("audio-volume-low",    notev[1].app_icon);
	EXPECT_EQ("audio-volume-low",    notev[2].app_icon);
	EXPECT_EQ("audio-volume-medium", notev[3].app_icon);
	EXPECT_EQ("audio-volume-medium", notev[4].app_icon);
	EXPECT_EQ("audio-volume-medium", notev[5].app_icon);
	EXPECT_EQ("audio-volume-medium", notev[6].app_icon);
	EXPECT_EQ("audio-volume-high",   notev[7].app_icon);
	EXPECT_EQ("audio-volume-high",   notev[8].app_icon);
	EXPECT_EQ("audio-volume-high",   notev[9].app_icon);
	EXPECT_EQ("audio-volume-high",   notev[10].app_icon);
}

TEST_F(NotificationsTest, ServerRestart) {
	auto options = optionsMock();
	auto volumeControl = volumeControlMock(options);
	auto soundService = standardService(volumeControl, playerListMock(), options);

	/* Set a volume */
	notifications->clearNotifications();
	setMockVolume(volumeControl, 0.50);
	loop(50);
	auto notev = notifications->getNotifications();
	ASSERT_EQ(1, notev.size());

	/* Restart server without sync notifications */
	notifications->clearNotifications();
	dbus_test_service_remove_task(service, (DbusTestTask*)*notifications);
	notifications.reset();

	loop(50);

	notifications = std::make_shared<NotificationsMock>(std::vector<std::string>({"body", "body-markup", "icon-static"}));
	dbus_test_service_add_task(service, (DbusTestTask*)*notifications);
	dbus_test_task_run((DbusTestTask*)*notifications);

	/* Change the volume */
	notifications->clearNotifications();
	setMockVolume(volumeControl, 0.60);
	loop(50);
	notev = notifications->getNotifications();
	ASSERT_EQ(0, notev.size());

	/* Put a good server back */
	dbus_test_service_remove_task(service, (DbusTestTask*)*notifications);
	notifications.reset();

	loop(50);

	notifications = std::make_shared<NotificationsMock>();
	dbus_test_service_add_task(service, (DbusTestTask*)*notifications);
	dbus_test_task_run((DbusTestTask*)*notifications);

	/* Change the volume again */
	notifications->clearNotifications();
	setMockVolume(volumeControl, 0.70);
	loop(50);
	notev = notifications->getNotifications();
	ASSERT_EQ(1, notev.size());
}

TEST_F(NotificationsTest, HighVolume) {
	auto options = optionsMock();
	auto volumeControl = volumeControlMock(options);
	auto soundService = standardService(volumeControl, playerListMock(), options);

	/* Set a volume */
	notifications->clearNotifications();
	setMockVolume(volumeControl, 0.50);
	loop(50);
	auto notev = notifications->getNotifications();
	ASSERT_EQ(1, notev.size());
	EXPECT_EQ("Volume", notev[0].summary);
	EXPECT_EQ("Speakers", notev[0].body);
	EXPECT_GVARIANT_EQ("@s 'false'", notev[0].hints["x-canonical-value-bar-tint"]);

	/* Set high volume with volume change */
	notifications->clearNotifications();
	volume_control_mock_set_high_volume(VOLUME_CONTROL_MOCK(volumeControl.get()), true);
	setMockVolume(volumeControl, 0.90);
	loop(50);
	notev = notifications->getNotifications();
	ASSERT_LT(0, notev.size()); /* This passes with one or two since it would just be an update to the first if a second was sent */
	EXPECT_EQ("Volume", notev[0].summary);
	EXPECT_EQ("Speakers", notev[0].body);
	EXPECT_GVARIANT_EQ("@s 'true'", notev[0].hints["x-canonical-value-bar-tint"]);

	/* Move it back */
	volume_control_mock_set_high_volume(VOLUME_CONTROL_MOCK(volumeControl.get()), false);
	setMockVolume(volumeControl, 0.50);
	loop(50);

	/* Set high volume without level change */
	/* NOTE: This can happen if headphones are plugged in */
	notifications->clearNotifications();
	volume_control_mock_set_high_volume(VOLUME_CONTROL_MOCK(volumeControl.get()), TRUE);
	loop(50);
	notev = notifications->getNotifications();
	ASSERT_EQ(1, notev.size());
	EXPECT_EQ("Volume", notev[0].summary);
	EXPECT_EQ("Speakers", notev[0].body);
	EXPECT_GVARIANT_EQ("@s 'true'", notev[0].hints["x-canonical-value-bar-tint"]);
}

TEST_F(NotificationsTest, MenuHide) {
	auto options = optionsMock();
	auto volumeControl = volumeControlMock(options);
	auto soundService = standardService(volumeControl, playerListMock(), options);

	/* Set a volume */
	notifications->clearNotifications();
	setMockVolume(volumeControl, 0.50);
	loop(50);
	auto notev = notifications->getNotifications();
	EXPECT_EQ(1, notev.size());

	/* Set the indicator to shown, and set a new volume */
	notifications->clearNotifications();
	setIndicatorShown(true);
	loop(50);
	setMockVolume(volumeControl, 0.60);
	loop(50);
	notev = notifications->getNotifications();
	EXPECT_EQ(0, notev.size());

	/* Set the indicator to hidden, and set a new volume */
	notifications->clearNotifications();
	setIndicatorShown(false);
	loop(50);
	setMockVolume(volumeControl, 0.70);
	loop(50);
	notev = notifications->getNotifications();
	EXPECT_EQ(1, notev.size());
}

TEST_F(NotificationsTest, DISABLED_ExtendendVolumeNotification) {
	auto options = optionsMock();
	auto volumeControl = volumeControlMock(options);
	auto soundService = standardService(volumeControl, playerListMock(), options);

	/* Set a volume */
	notifications->clearNotifications();
	setMockVolume(volumeControl, 0.50);
	loop(50);
	auto notev = notifications->getNotifications();
	ASSERT_EQ(1, notev.size());
	EXPECT_EQ("indicator-sound", notev[0].app_name);
	EXPECT_EQ("Volume", notev[0].summary);
	EXPECT_EQ(0, notev[0].actions.size());
	EXPECT_GVARIANT_EQ("@s 'true'", notev[0].hints["x-canonical-private-synchronous"]);
	EXPECT_GVARIANT_EQ("@i 50", notev[0].hints["value"]);

	/* Allow an amplified volume */
	notifications->clearNotifications();
	//indicator_sound_service_set_allow_amplified_volume(soundService.get(), TRUE);
	loop(50);
	notev = notifications->getNotifications();
	ASSERT_EQ(1, notev.size());
	EXPECT_GVARIANT_EQ("@i 33", notev[0].hints["value"]);

	/* Set to 'over max' */
	notifications->clearNotifications();
	setMockVolume(volumeControl, 1.525);
	loop(50);
	notev = notifications->getNotifications();
	ASSERT_EQ(1, notev.size());
	EXPECT_GVARIANT_EQ("@i 100", notev[0].hints["value"]);

	/* Put back */
	notifications->clearNotifications();
	//indicator_sound_service_set_allow_amplified_volume(soundService.get(), FALSE);
	loop(50);
	notev = notifications->getNotifications();
	ASSERT_EQ(1, notev.size());
	EXPECT_GVARIANT_EQ("@i 100", notev[0].hints["value"]);
}
