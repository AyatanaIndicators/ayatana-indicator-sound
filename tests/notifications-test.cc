/*
 * Copyright © 2015-2016 Canonical Ltd.
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
 *      Charles Kerr <charles.kerr@canonical.com>
 */

#include <algorithm>
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

        void loop_until(const std::function<bool()>& test, unsigned int max_ms=50, unsigned int test_interval_ms=10) {

            // g_timeout's callback only allows a single pointer,
            // so use a temporary stack struct to wedge everything into one pointer
            struct CallbackData {
                const std::function<bool()>& test;
                const gint64 deadline;
                GMainLoop* loop = g_main_loop_new(nullptr, false);
                CallbackData (const std::function<bool()>& f, unsigned int max_ms):
                    test{f},
                    deadline{g_get_monotonic_time() + (max_ms*1000)} {}
                ~CallbackData() {g_main_loop_unref(loop);}
            } data(test, max_ms);

            // tell the timer to stop looping on success or deadline
            auto timerfunc = [](gpointer gdata) -> gboolean {
                auto& data = *static_cast<CallbackData*>(gdata);
                if (!data.test() && (g_get_monotonic_time() < data.deadline))
                    return G_SOURCE_CONTINUE;
                g_main_loop_quit(data.loop);
                return G_SOURCE_REMOVE;
            };

            // start looping
            g_timeout_add (std::min(max_ms, test_interval_ms), timerfunc, &data);
            g_main_loop_run(data.loop);
        }

        void loop_until_notifications(unsigned int max_seconds=1) {
            auto test = [this]{ return !notifications->getNotifications().empty(); };
            loop_until(test, max_seconds);
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

        std::shared_ptr<VolumeWarning> volumeWarningMock (const std::shared_ptr<IndicatorSoundOptions>& optionsMock) {
            auto volumeWarning = std::shared_ptr<VolumeWarning>(
                VOLUME_WARNING(volume_warning_mock_new(optionsMock.get())),
                [](VolumeWarning * warning){
                    g_clear_object(&warning);
                });
            return volumeWarning;
        }

        std::shared_ptr<IndicatorSoundService> standardService (
                const std::shared_ptr<VolumeControl>& volumeControl,
                const std::shared_ptr<MediaPlayerList>& playerList,
                const std::shared_ptr<IndicatorSoundOptions>& options,
                const std::shared_ptr<VolumeWarning>& warning,
                const std::shared_ptr<AccountsServiceAccess>& accounts_service_access) {
            auto soundService = std::shared_ptr<IndicatorSoundService>(
                indicator_sound_service_new(playerList.get(), volumeControl.get(), nullptr, options.get(), warning.get(), accounts_service_access.get()),
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
    auto volumeWarning = volumeWarningMock(options);
    auto accountsService = std::make_shared<AccountsServiceAccess>();
    auto soundService = standardService(volumeControl, playerListMock(), options, volumeWarning, accountsService);

    /* Give some time settle */
    loop(50);

    /* Auto free */
}

TEST_F(NotificationsTest, VolumeChanges) {
    auto options = optionsMock();
    auto volumeControl = volumeControlMock(options);
    auto volumeWarning = volumeWarningMock(options);
    auto accountsService = std::make_shared<AccountsServiceAccess>();
    auto soundService = standardService(volumeControl, playerListMock(), options, volumeWarning, accountsService);

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
    auto volumeWarning = volumeWarningMock(options);
    auto accountsService = std::make_shared<AccountsServiceAccess>();
    auto soundService = standardService(volumeControl, playerListMock(), options, volumeWarning, accountsService);

    /* Set a volume */
    notifications->clearNotifications();
    setMockVolume(volumeControl, 0.5);
    loop(50);
    auto notev = notifications->getNotifications();
    ASSERT_EQ(1, notev.size());

    /* Change Streams, no volume change */
    notifications->clearNotifications();
    volume_control_mock_mock_set_active_stream(VOLUME_CONTROL_MOCK(volumeControl.get()), VOLUME_CONTROL_STREAM_ALARM);
    setMockVolume(volumeControl, 0.5, VOLUME_CONTROL_VOLUME_REASONS_VOLUME_STREAM_CHANGE);
    loop(50);
    notev = notifications->getNotifications();
    EXPECT_EQ(0, notev.size());

    /* Change Streams, volume change */
    notifications->clearNotifications();
    volume_control_mock_mock_set_active_stream(VOLUME_CONTROL_MOCK(volumeControl.get()), VOLUME_CONTROL_STREAM_ALERT);
    setMockVolume(volumeControl, 0.6, VOLUME_CONTROL_VOLUME_REASONS_VOLUME_STREAM_CHANGE);
    loop(50);
    notev = notifications->getNotifications();
    EXPECT_EQ(0, notev.size());

    /* Change Streams, no volume change, volume up */
    notifications->clearNotifications();
    volume_control_mock_mock_set_active_stream(VOLUME_CONTROL_MOCK(volumeControl.get()), VOLUME_CONTROL_STREAM_MULTIMEDIA);
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
    auto volumeWarning = volumeWarningMock(options);
    auto accountsService = std::make_shared<AccountsServiceAccess>();
    auto soundService = standardService(volumeControl, playerListMock(), options, volumeWarning, accountsService);

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
    auto volumeWarning = volumeWarningMock(options);
    auto accountsService = std::make_shared<AccountsServiceAccess>();
    auto soundService = standardService(volumeControl, playerListMock(), options, volumeWarning, accountsService);

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
    auto volumeWarning = volumeWarningMock(options);
    auto accountsService = std::make_shared<AccountsServiceAccess>();
    auto soundService = standardService(volumeControl, playerListMock(), options, volumeWarning, accountsService);

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
    volume_warning_mock_set_high_volume(VOLUME_WARNING_MOCK(volumeWarning.get()), true);
    setMockVolume(volumeControl, 0.90);
    loop(50);
    notev = notifications->getNotifications();
    ASSERT_LT(0, notev.size()); /* This passes with one or two since it would just be an update to the first if a second was sent */
    EXPECT_EQ("Volume", notev[0].summary);
    EXPECT_EQ("Speakers", notev[0].body);
    EXPECT_GVARIANT_EQ("@s 'true'", notev[0].hints["x-canonical-value-bar-tint"]);

    /* Move it back */
    volume_warning_mock_set_high_volume(VOLUME_WARNING_MOCK(volumeWarning.get()), false);
    setMockVolume(volumeControl, 0.50);
    loop(50);

    /* Set high volume without level change */
    /* NOTE: This can happen if headphones are plugged in */
    notifications->clearNotifications();
    volume_warning_mock_set_high_volume(VOLUME_WARNING_MOCK(volumeWarning.get()), true);
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
    auto volumeWarning = volumeWarningMock(options);
    auto accountsService = std::make_shared<AccountsServiceAccess>();
    auto soundService = standardService(volumeControl, playerListMock(), options, volumeWarning, accountsService);

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

TEST_F(NotificationsTest, ExtendendVolumeNotification) {
    auto options = optionsMock();
    auto volumeControl = volumeControlMock(options);
    auto volumeWarning = volumeWarningMock(options);
    auto accountsService = std::make_shared<AccountsServiceAccess>();
    auto soundService = standardService(volumeControl, playerListMock(), options, volumeWarning, accountsService);

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
    volume_control_mock_mock_set_active_stream(VOLUME_CONTROL_MOCK(volumeControl.get()), VOLUME_CONTROL_STREAM_ALARM);
    options_mock_mock_set_max_volume(OPTIONS_MOCK(options.get()), 1.5);
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
    options_mock_mock_set_max_volume(OPTIONS_MOCK(options.get()), 1.0);
    loop(50);
    notev = notifications->getNotifications();
    ASSERT_EQ(1, notev.size());
    EXPECT_GVARIANT_EQ("@i 100", notev[0].hints["value"]);
}

TEST_F(NotificationsTest, TriggerWarning) {

    // Tests all the conditions needed to trigger a volume warning.
    // There are many possible combinations, so this test is slow. :P

    const struct {
        bool expected;
        VolumeControlActiveOutput output;
    } test_outputs[] = {
        { false, VOLUME_CONTROL_ACTIVE_OUTPUT_SPEAKERS },
        { true,  VOLUME_CONTROL_ACTIVE_OUTPUT_HEADPHONES },
        { true,  VOLUME_CONTROL_ACTIVE_OUTPUT_BLUETOOTH_HEADPHONES },
        { false, VOLUME_CONTROL_ACTIVE_OUTPUT_BLUETOOTH_SPEAKER },
        { false, VOLUME_CONTROL_ACTIVE_OUTPUT_USB_SPEAKER },
        { true,  VOLUME_CONTROL_ACTIVE_OUTPUT_USB_HEADPHONES },
        { false, VOLUME_CONTROL_ACTIVE_OUTPUT_HDMI_SPEAKER },
        { true,  VOLUME_CONTROL_ACTIVE_OUTPUT_HDMI_HEADPHONES },
        { false, VOLUME_CONTROL_ACTIVE_OUTPUT_CALL_MODE }
    };

    const struct {
        bool expected;
        pa_volume_t volume;
        pa_volume_t loud_volume;
    } test_volumes[] = {
        { false,   50, 100 },
        { false,   99, 100 },
        { false,   100, 100 }, // Whenever you increase volume... such that acoustic output would be *MORE* than 85 dBA
        { true,   101, 100 }
    };

    const struct {
        bool expected;
        bool approved;
    } test_approved[] = {
        { true,  false },
        { false, true }
    };

    const struct {
        bool expected;
        bool warnings_enabled;
    } test_warnings_enabled[] = {
        { true,  true },
        { false, false }
    };

    const struct {
        bool expected;
        bool multimedia_active;
    } test_multimedia_active[] = {
        { true,  true },
        { false, false }
    };

    for (const auto& outputs : test_outputs) {
    for (const auto& volumes : test_volumes) {
    for (const auto& approved : test_approved) {
    for (const auto& warnings_enabled : test_warnings_enabled) {
    for (const auto& multimedia_active : test_multimedia_active) {

        notifications->clearNotifications();

        // instantiate the test subjects
        auto options = optionsMock();
        auto volumeControl = volumeControlMock(options);
        auto volumeWarning = volumeWarningMock(options);
        auto accountsService = std::make_shared<AccountsServiceAccess>();
        auto soundService = standardService(volumeControl, playerListMock(), options, volumeWarning, accountsService);

        // run the test
        options_mock_mock_set_loud_volume(OPTIONS_MOCK(options.get()), volumes.loud_volume);
        options_mock_mock_set_loud_warning_enabled(OPTIONS_MOCK(options.get()), warnings_enabled.warnings_enabled);
        volume_warning_mock_set_approved(VOLUME_WARNING_MOCK(volumeWarning.get()), approved.approved);
        volume_warning_mock_set_multimedia_volume(VOLUME_WARNING_MOCK(volumeWarning.get()), volumes.volume);
        volume_warning_mock_set_multimedia_active(VOLUME_WARNING_MOCK(volumeWarning.get()), multimedia_active.multimedia_active);
        volume_control_mock_mock_set_active_output(VOLUME_CONTROL_MOCK(volumeControl.get()), outputs.output);

        loop_until_notifications();

        // check the result
        auto notev = notifications->getNotifications();
        const bool warning_expected = outputs.expected && volumes.expected && approved.expected && warnings_enabled.expected && multimedia_active.expected;
        if (warning_expected) {
            EXPECT_TRUE(volume_warning_get_active(volumeWarning.get()));
            ASSERT_EQ(1, notev.size());
            EXPECT_GVARIANT_EQ("@s 'true'", notev[0].hints["x-canonical-snap-decisions"]);
            EXPECT_GVARIANT_EQ(nullptr, notev[0].hints["x-canonical-private-synchronous"]);
        }
        else {
            EXPECT_FALSE(volume_warning_get_active(volumeWarning.get()));
            ASSERT_EQ(1, notev.size());
            EXPECT_GVARIANT_EQ(nullptr, notev[0].hints["x-canonical-snap-decisions"]);
            EXPECT_GVARIANT_EQ("@s 'true'", notev[0].hints["x-canonical-private-synchronous"]);
        }

    } // multimedia_active
    } // warnings_enabled
    } // approved
    } // volumes
    } // outputs
}

