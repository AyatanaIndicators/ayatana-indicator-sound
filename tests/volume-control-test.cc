/*
 * Copyright 2014 Canonical Ltd.
 * Copyright 2021-2022 Robert Tari
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
 *      Robert Tari <robert@tari.in>
 */

#include <gtest/gtest.h>
#include <gio/gio.h>
#include <libdbustest/dbus-test.h>

extern "C" {
#include "indicator-sound-service.h"
#include "vala-mocks.h"
}

class VolumeControlTest : public ::testing::Test
{

    protected:
        DbusTestService * service = NULL;
        GDBusConnection * session = NULL;

        virtual void SetUp() override {

            g_setenv("GSETTINGS_SCHEMA_DIR", SCHEMA_DIR, TRUE);
            g_setenv("GSETTINGS_BACKEND", "memory", TRUE);

            service = dbus_test_service_new(NULL);
            dbus_test_service_start_tasks(service);

            session = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
            ASSERT_NE(nullptr, session);
            g_dbus_connection_set_exit_on_close(session, FALSE);
            g_object_add_weak_pointer(G_OBJECT(session), (gpointer *)&session);
        }

        virtual void TearDown() override {
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
            GMainLoop * pLoop = static_cast<GMainLoop *>(user_data);
            g_main_loop_quit(pLoop);
            return G_SOURCE_REMOVE;
        }

        void loop (unsigned int ms) {
            GMainLoop * loop = g_main_loop_new(NULL, FALSE);
            g_timeout_add(ms, timeout_cb, loop);
            g_main_loop_run(loop);
            g_main_loop_unref(loop);
        }
};

TEST_F(VolumeControlTest, DISABLED_BasicObject) {
    auto options = options_mock_new();
    auto pgloop = pa_glib_mainloop_new(NULL);
    auto accounts_service_access = accounts_service_access_new();
    auto control = volume_control_pulse_new(INDICATOR_SOUND_OPTIONS(options), pgloop, accounts_service_access);

    /* Setup the PA backend */
    loop(100);

    /* Ready */
    EXPECT_TRUE(volume_control_get_ready(VOLUME_CONTROL(control)));

    g_clear_object(&control);
    g_clear_object(&options);
    g_clear_pointer(&pgloop, pa_glib_mainloop_free);
}
