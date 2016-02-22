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

extern "C" {
#include "indicator-sound-service.h"
#include "vala-mocks.h"
}

class SoundMenuTest : public ::testing::Test
{
    protected:
        GTestDBus * bus = nullptr;

        virtual void SetUp() {
            bus = g_test_dbus_new(G_TEST_DBUS_NONE);
            g_test_dbus_up(bus);
        }

        virtual void TearDown() {
            g_test_dbus_down(bus);
            g_clear_object(&bus);
        }

        void verify_item_attribute (GMenuModel * mm, guint index, const gchar * name, GVariant * value) {
            g_variant_ref_sink(value);

            gchar * variantstr = g_variant_print(value, TRUE);
            g_debug("Expecting item %d to have a '%s' attribute: %s", index, name, variantstr);

            const GVariantType * type = g_variant_get_type(value);
            GVariant * itemval = g_menu_model_get_item_attribute_value(mm, index, name, type);

            ASSERT_NE(nullptr, itemval);
            EXPECT_TRUE(g_variant_equal(itemval, value));

            g_variant_unref(value);
        }

        void verify_item_attribute_is_not_set(GMenuModel * mm, guint index, const gchar * name, const GVariantType * type) {
            GVariant * itemval = g_menu_model_get_item_attribute_value(mm, index, name, type);
            EXPECT_EQ(itemval, nullptr);
        }

        void check_player_control_buttons(bool canPlay, bool canNext, bool canPrev)
        {
            SoundMenu * menu = sound_menu_new (nullptr, SOUND_MENU_DISPLAY_FLAGS_NONE);

            MediaPlayerTrack * track = media_player_track_new("Artist", "Title", "Album", "http://art.url");

            MediaPlayerMock * media = MEDIA_PLAYER_MOCK(
                g_object_new(TYPE_MEDIA_PLAYER_MOCK,
                    "mock-id", "player-id",
                    "mock-name", "Test Player",
                    "mock-state", "Playing",
                    "mock-is-running", TRUE,
                    "mock-can-raise", FALSE,
                    "mock-current-track", track,
                    "mock-can-do-play", canPlay,
                    "mock-can-do-next", canNext,
                    "mock-can-do-prev", canPrev,
                    NULL)
            );
            g_clear_object(&track);

            sound_menu_add_player(menu, MEDIA_PLAYER(media));

            ASSERT_NE(nullptr, menu->menu);
            EXPECT_EQ(2, g_menu_model_get_n_items(G_MENU_MODEL(menu->menu)));

            GMenuModel * section = g_menu_model_get_item_link(G_MENU_MODEL(menu->menu), 1, G_MENU_LINK_SECTION);
            ASSERT_NE(nullptr, section);
            EXPECT_EQ(2, g_menu_model_get_n_items(section)); /* No playlists, so two items */

            /* Player display */
            verify_item_attribute(section, 0, "action", g_variant_new_string("indicator.player-id"));
            verify_item_attribute(section, 0, "x-canonical-type", g_variant_new_string("com.canonical.unity.media-player"));

            /* Player control */
            verify_item_attribute(section, 1, "x-canonical-type", g_variant_new_string("com.canonical.unity.playback-item"));
        //verify_item_attribute(section, 1, "x-canonical-play-action", g_variant_new_string(""));
        if (!canPlay) {
                verify_item_attribute_is_not_set(section, 1, "x-canonical-play-action", G_VARIANT_TYPE_STRING);
        } else {
            verify_item_attribute(section, 1, "x-canonical-play-action", g_variant_new_string("indicator.play.player-id"));
        }
        if (!canNext) {
                verify_item_attribute_is_not_set(section, 1, "x-canonical-next-action", G_VARIANT_TYPE_STRING);
        } else {
            verify_item_attribute(section, 1, "x-canonical-next-action", g_variant_new_string("indicator.next.player-id"));
        }
        if (!canPrev) {
                verify_item_attribute_is_not_set(section, 1, "x-canonical-previous-action", G_VARIANT_TYPE_STRING);
        } else {
            verify_item_attribute(section, 1, "x-canonical-previous-action", g_variant_new_string("indicator.previous.player-id"));
        }

            g_clear_object(&section);

            sound_menu_remove_player(menu, MEDIA_PLAYER(media));

            EXPECT_EQ(1, g_menu_model_get_n_items(G_MENU_MODEL(menu->menu)));

            g_clear_object(&media);
            g_clear_object(&menu);
        }
};

TEST_F(SoundMenuTest, BasicObject) {
    SoundMenu * menu = sound_menu_new (nullptr, SOUND_MENU_DISPLAY_FLAGS_NONE);

    ASSERT_NE(nullptr, menu);

    g_clear_object(&menu);
    return;
}

TEST_F(SoundMenuTest, AddRemovePlayer) {
    SoundMenu * menu = sound_menu_new (nullptr, SOUND_MENU_DISPLAY_FLAGS_NONE);

    MediaPlayerTrack * track = media_player_track_new("Artist", "Title", "Album", "http://art.url");

    MediaPlayerMock * media = MEDIA_PLAYER_MOCK(
        g_object_new(TYPE_MEDIA_PLAYER_MOCK,
            "mock-id", "player-id",
            "mock-name", "Test Player",
            "mock-state", "Playing",
            "mock-is-running", TRUE,
            "mock-can-raise", FALSE,
            "mock-current-track", track,
            "mock-can-do-play", TRUE,
            "mock-can-do-next", TRUE,
            "mock-can-do-prev", TRUE,
            NULL)
    );
    g_clear_object(&track);

    sound_menu_add_player(menu, MEDIA_PLAYER(media));

    ASSERT_NE(nullptr, menu->menu);
    EXPECT_EQ(2, g_menu_model_get_n_items(G_MENU_MODEL(menu->menu)));

    GMenuModel * section = g_menu_model_get_item_link(G_MENU_MODEL(menu->menu), 1, G_MENU_LINK_SECTION);
    ASSERT_NE(nullptr, section);
    EXPECT_EQ(2, g_menu_model_get_n_items(section)); /* No playlists, so two items */

    /* Player display */
    verify_item_attribute(section, 0, "action", g_variant_new_string("indicator.player-id"));
    verify_item_attribute(section, 0, "x-canonical-type", g_variant_new_string("com.canonical.unity.media-player"));

    /* Player control */
    verify_item_attribute(section, 1, "x-canonical-type", g_variant_new_string("com.canonical.unity.playback-item"));
    verify_item_attribute(section, 1, "x-canonical-play-action", g_variant_new_string("indicator.play.player-id"));
    verify_item_attribute(section, 1, "x-canonical-next-action", g_variant_new_string("indicator.next.player-id"));
    verify_item_attribute(section, 1, "x-canonical-previous-action", g_variant_new_string("indicator.previous.player-id"));

    g_clear_object(&section);

    sound_menu_remove_player(menu, MEDIA_PLAYER(media));

    EXPECT_EQ(1, g_menu_model_get_n_items(G_MENU_MODEL(menu->menu)));

    g_clear_object(&media);
    g_clear_object(&menu);
    return;
}

TEST_F(SoundMenuTest, AddRemovePlayerNoPlayNextPrev) {
    check_player_control_buttons(false, false, false);
}

TEST_F(SoundMenuTest, AddRemovePlayerNoNext) {
    check_player_control_buttons(true, false, true);
}

TEST_F(SoundMenuTest, AddRemovePlayerNoPrev) {
    check_player_control_buttons(true, true, false);
}

TEST_F(SoundMenuTest, AddRemovePlayerNoPlay) {
    check_player_control_buttons(false, true, true);
}

//
