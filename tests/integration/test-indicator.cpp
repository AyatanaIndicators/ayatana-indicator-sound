/*
 * Copyright (C) 2015 Canonical, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Xavi Garcia <xavi.garcia.mena@canonical.com>
 */

#include <indicator-sound-test-base.h>

#include <QDebug>
#include <QTestEventLoop>
#include <QSignalSpy>

using namespace std;
using namespace testing;
namespace mh = unity::gmenuharness;
namespace
{

class TestIndicator: public IndicatorSoundTestBase
{
};

TEST_F(TestIndicator, PhoneChangeRoleVolume)
{
    double INITIAL_VOLUME = 0.0;

    ASSERT_NO_THROW(startAccountsService());
    ASSERT_NO_THROW(startPulsePhone());

    // initialize volumes in pulseaudio
    EXPECT_TRUE(setStreamRestoreVolume("alert", INITIAL_VOLUME));

    // start now the indicator, so it picks the new volumes
    ASSERT_NO_THROW(startIndicator());

    // Generate a random volume
    QTime now = QTime::currentTime();
    qsrand(now.msec());
    int randInt = qrand() % 100;
    double randomVolume = randInt / 100.0;

    // set an initial volume to the alert role
    setStreamRestoreVolume("alert", 1.0);
    EXPECT_TRUE(waitVolumeChangedInIndicator());

    // play a test sound, it should change the role in the indicator
    EXPECT_TRUE(startTestSound("multimedia"));
    EXPECT_TRUE(waitVolumeChangedInIndicator());

    // set the random volume to the multimedia role
    EXPECT_TRUE(setStreamRestoreVolume("multimedia", randomVolume));
    if (randomVolume != INITIAL_VOLUME)
    {
        EXPECT_TRUE(waitVolumeChangedInIndicator());
    }

    // check the indicator
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .action("indicator.root")
            .string_attribute("x-canonical-type", "com.canonical.indicator.root")
            .string_attribute("x-canonical-scroll-action", "indicator.scroll")
            .string_attribute("x-canonical-secondary-action", "indicator.mute")
            .string_attribute("submenu-action", "indicator.indicator-shown")
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .submenu()
            .item(mh::MenuItemMatcher()
                .section()
                .item(silentModeSwitch(false))
                .item(volumeSlider(randomVolume))
            )
        ).match());

    // check that the last item is Sound Settings
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .action("indicator.root")
            .string_attribute("x-canonical-type", "com.canonical.indicator.root")
            .string_attribute("x-canonical-secondary-action", "indicator.mute")
            .mode(mh::MenuItemMatcher::Mode::ends_with)
            .submenu()
            .item(mh::MenuItemMatcher()
                .label("Sound Settings…")
                .action("indicator.phone-settings")
            )
        ).match());

    // stop the test sound, the role should change again to alert
    stopTestSound();
    if (randomVolume != 1.0)
    {
        // we only wait if the volume in the alert and the
        // one set in the multimedia roles differ
        EXPECT_TRUE(waitVolumeChangedInIndicator());
    }

    // check the initial volume for the alert role
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .action("indicator.root")
            .string_attribute("x-canonical-type", "com.canonical.indicator.root")
            .string_attribute("x-canonical-scroll-action", "indicator.scroll")
            .string_attribute("x-canonical-secondary-action", "indicator.mute")
            .string_attribute("submenu-action", "indicator.indicator-shown")
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .submenu()
            .item(mh::MenuItemMatcher()
                .section()
                .item(silentModeSwitch(false))
                .item(volumeSlider(1.0))
            )
        ).match());

    // check that the last item is Sound Settings
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .action("indicator.root")
            .string_attribute("x-canonical-type", "com.canonical.indicator.root")
            .string_attribute("x-canonical-secondary-action", "indicator.mute")
            .mode(mh::MenuItemMatcher::Mode::ends_with)
            .submenu()
            .item(mh::MenuItemMatcher()
                .label("Sound Settings…")
                .action("indicator.phone-settings")
            )
        ).match());
}

TEST_F(TestIndicator, PhoneBasicInitialVolume)
{
    double INITIAL_VOLUME = 0.0;

    ASSERT_NO_THROW(startAccountsService());
    EXPECT_TRUE(clearGSettingsPlayers());
    ASSERT_NO_THROW(startPulsePhone());

    // initialize volumes in pulseaudio
    EXPECT_TRUE(setStreamRestoreVolume("alert", INITIAL_VOLUME));

    // start now the indicator, so it picks the new volumes
    ASSERT_NO_THROW(startIndicator());

    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .action("indicator.root")
            .string_attribute("x-canonical-type", "com.canonical.indicator.root")
            .string_attribute("x-canonical-scroll-action", "indicator.scroll")
            .string_attribute("x-canonical-secondary-action", "indicator.mute")
            .string_attribute("submenu-action", "indicator.indicator-shown")
            .mode(mh::MenuItemMatcher::Mode::all)
            .submenu()
            .item(mh::MenuItemMatcher()
                .section()
                .item(silentModeSwitch(false))
                .item(volumeSlider(INITIAL_VOLUME))
            )
//            .item(mh::MenuItemMatcher()
//                .section()
//                .action("indicator.testplayer1.desktop")
//            )
            .item(mh::MenuItemMatcher()
                .label("Sound Settings…")
                .action("indicator.phone-settings")
            )
        ).match());
}

TEST_F(TestIndicator, DesktopBasicInitialVolume)
{
    double INITIAL_VOLUME = 0.0;

    ASSERT_NO_THROW(startAccountsService());
    EXPECT_TRUE(clearGSettingsPlayers());
    ASSERT_NO_THROW(startPulseDesktop());

    // initialize volumes in pulseaudio
    EXPECT_FALSE(setStreamRestoreVolume("alert", INITIAL_VOLUME));
    EXPECT_TRUE(setSinkVolume(INITIAL_VOLUME));

    // start now the indicator, so it picks the new volumes
    ASSERT_NO_THROW(startIndicator());

    EXPECT_MATCHRESULT(mh::MenuMatcher(desktopParameters())
        .item(mh::MenuItemMatcher()
            .action("indicator.root")
            .string_attribute("x-canonical-type", "com.canonical.indicator.root")
            .string_attribute("x-canonical-secondary-action", "indicator.mute")
            .mode(mh::MenuItemMatcher::Mode::all)
            .submenu()
            .item(mh::MenuItemMatcher()
                .section()
                .item(mh::MenuItemMatcher().checkbox()
                    .label("Mute")
                )
                .item(volumeSlider(INITIAL_VOLUME))
            )
            .item(mh::MenuItemMatcher()
                            .label("Sound Settings…")
             )
        ).match());
}

TEST_F(TestIndicator, DesktopAddPlayer)
{
    double INITIAL_VOLUME = 0.0;

    ASSERT_NO_THROW(startAccountsService());
    EXPECT_TRUE(clearGSettingsPlayers());
    ASSERT_NO_THROW(startPulseDesktop());

    // initialize volumes in pulseaudio
    EXPECT_FALSE(setStreamRestoreVolume("alert", INITIAL_VOLUME));
    EXPECT_TRUE(setSinkVolume(INITIAL_VOLUME));

    // start the test player
    EXPECT_TRUE(startTestMprisPlayer("testplayer1"));

    // start now the indicator, so it picks the new volumes
    ASSERT_NO_THROW(startIndicator());

    // check that the player is added
    EXPECT_MATCHRESULT(mh::MenuMatcher(desktopParameters())
        .item(mh::MenuItemMatcher()
            .action("indicator.root")
            .string_attribute("x-canonical-type", "com.canonical.indicator.root")
            .string_attribute("x-canonical-secondary-action", "indicator.mute")
            .mode(mh::MenuItemMatcher::Mode::all)
            .submenu()
            .item(mh::MenuItemMatcher()
                .section()
                .item(mh::MenuItemMatcher().checkbox()
                    .label("Mute")
                )
                .item(volumeSlider(INITIAL_VOLUME))
            )
            .item(mh::MenuItemMatcher()
                .section()
                .item(mh::MenuItemMatcher()
                    .action("indicator.testplayer1.desktop")
                    .label("TestPlayer1")
                    .themed_icon("icon", {"testplayer"})
                    .string_attribute("x-canonical-type", "com.canonical.unity.media-player")
                )
                .item(mh::MenuItemMatcher()
                    .string_attribute("x-canonical-previous-action","indicator.previous.testplayer1.desktop")
                    .string_attribute("x-canonical-play-action","indicator.play.testplayer1.desktop")
                    .string_attribute("x-canonical-next-action","indicator.next.testplayer1.desktop")
                    .string_attribute("x-canonical-type","com.canonical.unity.playback-item")
                )
            )
            .item(mh::MenuItemMatcher()
                            .label("Sound Settings…")
             )
        ).match());
}

TEST_F(TestIndicator, DesktopChangeRoleVolume)
{
    double INITIAL_VOLUME = 0.0;

    ASSERT_NO_THROW(startAccountsService());
    ASSERT_NO_THROW(startPulseDesktop());

    // initialize volumes in pulseaudio
    // expect false for stream restore, because the module
    // is not loaded in the desktop pulseaudio instance
    EXPECT_FALSE(setStreamRestoreVolume("mutimedia", INITIAL_VOLUME));
    EXPECT_FALSE(setStreamRestoreVolume("alert", INITIAL_VOLUME));

    EXPECT_TRUE(setSinkVolume(INITIAL_VOLUME));

    // start now the indicator, so it picks the new volumes
    ASSERT_NO_THROW(startIndicator());

    // Generate a random volume
    QTime now = QTime::currentTime();
    qsrand(now.msec());
    int randInt = qrand() % 100;
    double randomVolume = randInt / 100.0;

    // play a test sound, it should NOT change the role in the indicator
    EXPECT_TRUE(startTestSound("multimedia"));
    EXPECT_FALSE(waitVolumeChangedInIndicator());

    // set the random volume
    EXPECT_TRUE(setSinkVolume(randomVolume));
    if (randomVolume != INITIAL_VOLUME)
    {
        EXPECT_FALSE(waitVolumeChangedInIndicator());
    }

    // check the indicator
    EXPECT_MATCHRESULT(mh::MenuMatcher(desktopParameters())
        .item(mh::MenuItemMatcher()
            .action("indicator.root")
            .string_attribute("x-canonical-type", "com.canonical.indicator.root")
            .string_attribute("x-canonical-secondary-action", "indicator.mute")
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .submenu()
            .item(mh::MenuItemMatcher()
                .section()
                .item(mh::MenuItemMatcher().checkbox()
                    .label("Mute")
                )
                .item(volumeSlider(randomVolume))
            )
        ).match());

    // check that the last item is Sound Settings
    EXPECT_MATCHRESULT(mh::MenuMatcher(desktopParameters())
        .item(mh::MenuItemMatcher()
            .action("indicator.root")
            .string_attribute("x-canonical-type", "com.canonical.indicator.root")
            .string_attribute("x-canonical-secondary-action", "indicator.mute")
            .mode(mh::MenuItemMatcher::Mode::ends_with)
            .submenu()
            .item(mh::MenuItemMatcher()
                .label("Sound Settings…")
            )
        ).match());

    // stop the test sound, the role should change again to alert
    stopTestSound();

    // although we were playing something in the multimedia role
    // the server does not have the streamrestore module, so
    // the volume does not change (the role does not change)
    EXPECT_FALSE(waitVolumeChangedInIndicator());

    // check the initial volume for the alert role
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .action("indicator.root")
            .string_attribute("x-canonical-type", "com.canonical.indicator.root")
            .string_attribute("x-canonical-scroll-action", "indicator.scroll")
            .string_attribute("x-canonical-secondary-action", "indicator.mute")
            .string_attribute("submenu-action", "indicator.indicator-shown")
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .submenu()
            .item(mh::MenuItemMatcher()
                .section()
                .item(silentModeSwitch(false))
                .item(volumeSlider(randomVolume))
            )
        ).match());

    // check that the last item is Sound Settings
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .action("indicator.root")
            .string_attribute("x-canonical-type", "com.canonical.indicator.root")
            .string_attribute("x-canonical-secondary-action", "indicator.mute")
            .mode(mh::MenuItemMatcher::Mode::ends_with)
            .submenu()
            .item(mh::MenuItemMatcher()
                .label("Sound Settings…")
                .action("indicator.phone-settings")
            )
        ).match());
}

} // namespace
