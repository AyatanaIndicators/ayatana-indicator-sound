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

TEST_F(TestIndicator, PhoneCheckRootIcon)
{
    double INITIAL_VOLUME = 0.0;

    ASSERT_NO_THROW(startAccountsService());
    EXPECT_TRUE(clearGSettingsPlayers());
    ASSERT_NO_THROW(startPulsePhone());

    // initialize volumes in pulseaudio
    EXPECT_TRUE(setStreamRestoreVolume("alert", INITIAL_VOLUME));

    // start now the indicator, so it picks the new volumes
    ASSERT_NO_THROW(startIndicator());

    QStringList mutedIcon = {"audio-volume-muted-panel", "audio-volume-muted", "audio-volume", "audio"};
    EXPECT_EQ(getRootIconValue(), mutedIcon);

    QStringList lowVolumeIcon = {"audio-volume-low-panel", "audio-volume-low", "audio-volume", "audio"};
    for( double volume = 0.1; volume <= 0.3; volume+=0.1)
    {
        EXPECT_TRUE(setStreamRestoreVolume("alert", volume));
        EXPECT_EQ(getRootIconValue(), lowVolumeIcon);
    }
    EXPECT_TRUE(setStreamRestoreVolume("alert", 0.4));

    QStringList mediumVolumeIcon = {"audio-volume-medium-panel", "audio-volume-medium", "audio-volume", "audio"};
    for( double volume = 0.4; volume <= 0.7; volume+=0.1)
    {
        EXPECT_TRUE(setStreamRestoreVolume("alert", volume));
        EXPECT_EQ(getRootIconValue(), mediumVolumeIcon);
    }

    QStringList highVolumeIcon = {"audio-volume-high-panel", "audio-volume-high", "audio-volume", "audio"};
    for( double volume = 0.8; volume <= 1.0; volume+=0.1)
    {
        EXPECT_TRUE(setStreamRestoreVolume("alert", volume));
        EXPECT_EQ(getRootIconValue(), highVolumeIcon);
    }
}

TEST_F(TestIndicator, PhoneTestExternalMicInOut)
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
                .item(volumeSlider(INITIAL_VOLUME, "Volume"))
            )
            .item(mh::MenuItemMatcher()
                .label("Sound Settings…")
                .action("indicator.phone-settings")
            )
        ).match());

    EXPECT_TRUE(plugExternalMic(true));

    // check that we have the mic slider now.
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
                .item(volumeSlider(INITIAL_VOLUME, "Volume"))
                .item(micSlider(1.0, "Microphone Volume"))
            )
            .item(mh::MenuItemMatcher()
                .label("Sound Settings…")
                .action("indicator.phone-settings")
            )
        ).match());


    // unplug the external mic
    EXPECT_TRUE(plugExternalMic(false));

    // and check that there's no mic slider
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
                .item(volumeSlider(INITIAL_VOLUME, "Volume"))
            )
            .item(mh::MenuItemMatcher()
                .label("Sound Settings…")
                .action("indicator.phone-settings")
            )
        ).match());
}

TEST_F(TestIndicator, DesktopTestExternalMicInOut)
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
                .item(volumeSlider(INITIAL_VOLUME, "Volume"))
            )
            .item(mh::MenuItemMatcher()
                            .label("Sound Settings…")
             )
        ).match());

    EXPECT_TRUE(plugExternalMic(true));

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
                .item(volumeSlider(INITIAL_VOLUME, "Volume"))
                .item(micSlider(1.0, "Microphone Volume"))
            )
            .item(mh::MenuItemMatcher()
                            .label("Sound Settings…")
             )
        ).match());

    EXPECT_TRUE(plugExternalMic(false));

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
                   .item(volumeSlider(INITIAL_VOLUME, "Volume"))
               )
               .item(mh::MenuItemMatcher()
                               .label("Sound Settings…")
                )
           ).match());
}

TEST_F(TestIndicator, DISABLED_PhoneChangeRoleVolume)
{
    double INITIAL_VOLUME = 0.0;

    ASSERT_NO_THROW(startAccountsService());
    ASSERT_NO_THROW(startPulsePhone());

    // initialize volumes in pulseaudio
    EXPECT_TRUE(setStreamRestoreVolume("alert", INITIAL_VOLUME));
    EXPECT_TRUE(setStreamRestoreVolume("multimedia", INITIAL_VOLUME));

    // start now the indicator, so it picks the new volumes
    ASSERT_NO_THROW(startIndicator());

    // Generate a random volume in the range [0...0.33]
    QTime now = QTime::currentTime();
    qsrand(now.msec());
    int randInt = qrand() % 33;
    const double randomVolume = randInt / 100.0;

    QSignalSpy &userAccountsSpy = *signal_spy_volume_changed_;
    // set an initial volume to the alert role
    userAccountsSpy.clear();
    EXPECT_TRUE(setVolumeUntilAccountsIsConnected(1.0));
    userAccountsSpy.clear();
    // play a test sound, it should change the role in the indicator
    EXPECT_TRUE(startTestSound("multimedia"));

    // this time we only expect 1 signal as it's only the indicator
    // updating the value
    WAIT_FOR_SIGNALS(userAccountsSpy, 1);
    //EXPECT_TRUE(waitVolumeChangedInIndicator());

    userAccountsSpy.clear();
    // set the random volume to the multimedia role
    setActionValue("volume", QVariant::fromValue(randomVolume));
    if (randomVolume != INITIAL_VOLUME)
    {
        WAIT_FOR_SIGNALS(userAccountsSpy, 1);
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
                .item(volumeSlider(randomVolume, "Volume"))
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

    // initialize the signal spy
    EXPECT_TRUE(initializeMenuChangedSignal());
    userAccountsSpy.clear();
    // stop the test sound, the role should change again to alert
    stopTestSound();
    if (randomVolume != 1.0)
    {
        // wait for the menu change
        EXPECT_TRUE(waitMenuChange());
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
                .item(volumeSlider(1.0, "Volume"))
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
                .item(volumeSlider(INITIAL_VOLUME, "Volume"))
            )
            .item(mh::MenuItemMatcher()
                .label("Sound Settings…")
                .action("indicator.phone-settings")
            )
        ).match());
}

TEST_F(TestIndicator, PhoneAddMprisPlayer)
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
                .item(volumeSlider(INITIAL_VOLUME, "Volume"))
            )
            .item(mh::MenuItemMatcher()
                .label("Sound Settings…")
                .action("indicator.phone-settings")
            )
        ).match());

    // initialize the signal spy
    EXPECT_TRUE(initializeMenuChangedSignal());

    // start the test player
    EXPECT_TRUE(startTestMprisPlayer("testplayer1"));

    // wait for the menu change
    EXPECT_TRUE(waitMenuChange());

    // finally verify that the player is added
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
                .item(volumeSlider(INITIAL_VOLUME, "Volume"))
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

    // start the test player
    EXPECT_TRUE(startTestMprisPlayer("testplayer1"));

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
                .item(volumeSlider(INITIAL_VOLUME, "Volume"))
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

TEST_F(TestIndicator, DesktopAddMprisPlayer)
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
                .item(volumeSlider(INITIAL_VOLUME, "Volume"))
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

    // stop the test player
    EXPECT_TRUE(stopTestMprisPlayer("testplayer1"));

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
                    .item(volumeSlider(INITIAL_VOLUME, "Volume"))
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
                        .string_attribute("x-canonical-play-action","indicator.play.testplayer1.desktop")
                        .string_attribute("x-canonical-type","com.canonical.unity.playback-item")
                    )
                )
                .item(mh::MenuItemMatcher()
                                .label("Sound Settings…")
                 )
            ).match());
}

TEST_F(TestIndicator, DesktopMprisPlayersPlaybackControls)
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
                .item(volumeSlider(INITIAL_VOLUME, "Volume"))
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

    // start the second test player
    EXPECT_TRUE(startTestMprisPlayer("testplayer2"));

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
                .item(volumeSlider(INITIAL_VOLUME, "Volume"))
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
                        .section()
                        .item(mh::MenuItemMatcher()
                            .action("indicator.testplayer2.desktop")
                            .label("TestPlayer2")
                            .themed_icon("icon", {"testplayer"})
                            .string_attribute("x-canonical-type", "com.canonical.unity.media-player")
                        )
                        .item(mh::MenuItemMatcher()
                            .string_attribute("x-canonical-previous-action","indicator.previous.testplayer2.desktop")
                            .string_attribute("x-canonical-play-action","indicator.play.testplayer2.desktop")
                            .string_attribute("x-canonical-next-action","indicator.next.testplayer2.desktop")
                            .string_attribute("x-canonical-type","com.canonical.unity.playback-item")
                        )
                    )
            .item(mh::MenuItemMatcher()
                            .label("Sound Settings…")
             )
        ).match());

    // start the third test player
    EXPECT_TRUE(startTestMprisPlayer("testplayer3"));

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
                .item(volumeSlider(INITIAL_VOLUME, "Volume"))
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
                        .section()
                        .item(mh::MenuItemMatcher()
                            .action("indicator.testplayer2.desktop")
                            .label("TestPlayer2")
                            .themed_icon("icon", {"testplayer"})
                            .string_attribute("x-canonical-type", "com.canonical.unity.media-player")
                        )
                        .item(mh::MenuItemMatcher()
                            .string_attribute("x-canonical-previous-action","indicator.previous.testplayer2.desktop")
                            .string_attribute("x-canonical-play-action","indicator.play.testplayer2.desktop")
                            .string_attribute("x-canonical-next-action","indicator.next.testplayer2.desktop")
                            .string_attribute("x-canonical-type","com.canonical.unity.playback-item")
                        )
                    )
            .item(mh::MenuItemMatcher()
                        .section()
                        .item(mh::MenuItemMatcher()
                            .action("indicator.testplayer3.desktop")
                            .label("TestPlayer3")
                            .themed_icon("icon", {"testplayer"})
                            .string_attribute("x-canonical-type", "com.canonical.unity.media-player")
                        )
                        .item(mh::MenuItemMatcher()
                            .string_attribute("x-canonical-previous-action","indicator.previous.testplayer3.desktop")
                            .string_attribute("x-canonical-play-action","indicator.play.testplayer3.desktop")
                            .string_attribute("x-canonical-next-action","indicator.next.testplayer3.desktop")
                            .string_attribute("x-canonical-type","com.canonical.unity.playback-item")
                        )
                    )
            .item(mh::MenuItemMatcher()
                            .label("Sound Settings…")
             )
        ).match());

    // stop the test player
    EXPECT_TRUE(stopTestMprisPlayer("testplayer3"));

    // check that player 3 is present, but it has no playback controls
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
                .item(volumeSlider(INITIAL_VOLUME, "Volume"))
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
                        .section()
                        .item(mh::MenuItemMatcher()
                            .action("indicator.testplayer2.desktop")
                            .label("TestPlayer2")
                            .themed_icon("icon", {"testplayer"})
                            .string_attribute("x-canonical-type", "com.canonical.unity.media-player")
                        )
                        .item(mh::MenuItemMatcher()
                            .string_attribute("x-canonical-previous-action","indicator.previous.testplayer2.desktop")
                            .string_attribute("x-canonical-play-action","indicator.play.testplayer2.desktop")
                            .string_attribute("x-canonical-next-action","indicator.next.testplayer2.desktop")
                            .string_attribute("x-canonical-type","com.canonical.unity.playback-item")
                        )
                    )
            .item(mh::MenuItemMatcher()
                        .section()
                        .item(mh::MenuItemMatcher()
                            .action("indicator.testplayer3.desktop")
                            .label("TestPlayer3")
                            .themed_icon("icon", {"testplayer"})
                            .string_attribute("x-canonical-type", "com.canonical.unity.media-player")
                        )
                    )
            .item(mh::MenuItemMatcher()
                            .label("Sound Settings…")
             )
        ).match());

    EXPECT_TRUE(stopTestMprisPlayer("testplayer2"));

    // check that player 2 is present, but it has no playback controls
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
                .item(volumeSlider(INITIAL_VOLUME, "Volume"))
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
                        .section()
                        .item(mh::MenuItemMatcher()
                            .action("indicator.testplayer2.desktop")
                            .label("TestPlayer2")
                            .themed_icon("icon", {"testplayer"})
                            .string_attribute("x-canonical-type", "com.canonical.unity.media-player")
                        )
                    )
            .item(mh::MenuItemMatcher()
                        .section()
                        .item(mh::MenuItemMatcher()
                            .action("indicator.testplayer3.desktop")
                            .label("TestPlayer3")
                            .themed_icon("icon", {"testplayer"})
                            .string_attribute("x-canonical-type", "com.canonical.unity.media-player")
                        )
                    )
            .item(mh::MenuItemMatcher()
                            .label("Sound Settings…")
             )
        ).match());

    EXPECT_TRUE(stopTestMprisPlayer("testplayer1"));

    // check that player 1 is present, and it still has the playback controls
    // as it was the last one being executed
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
                .item(volumeSlider(INITIAL_VOLUME, "Volume"))
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
                        .section()
                        .item(mh::MenuItemMatcher()
                            .action("indicator.testplayer2.desktop")
                            .label("TestPlayer2")
                            .themed_icon("icon", {"testplayer"})
                            .string_attribute("x-canonical-type", "com.canonical.unity.media-player")
                        )
                    )
            .item(mh::MenuItemMatcher()
                        .section()
                        .item(mh::MenuItemMatcher()
                            .action("indicator.testplayer3.desktop")
                            .label("TestPlayer3")
                            .themed_icon("icon", {"testplayer"})
                            .string_attribute("x-canonical-type", "com.canonical.unity.media-player")
                        )
                    )
            .item(mh::MenuItemMatcher()
                            .label("Sound Settings…")
             )
        ).match());

    // start the third test player
    EXPECT_TRUE(startTestMprisPlayer("testplayer3"));

    // check that player 3 is the only one with playback controls
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
                .item(volumeSlider(INITIAL_VOLUME, "Volume"))
            )
            .item(mh::MenuItemMatcher()
                    .section()
                    .item(mh::MenuItemMatcher()
                        .action("indicator.testplayer1.desktop")
                        .label("TestPlayer1")
                        .themed_icon("icon", {"testplayer"})
                        .string_attribute("x-canonical-type", "com.canonical.unity.media-player")
                    )
            )
            .item(mh::MenuItemMatcher()
                    .section()
                    .item(mh::MenuItemMatcher()
                        .action("indicator.testplayer2.desktop")
                        .label("TestPlayer2")
                        .themed_icon("icon", {"testplayer"})
                        .string_attribute("x-canonical-type", "com.canonical.unity.media-player")
                    )
                )
            .item(mh::MenuItemMatcher()
                    .section()
                    .item(mh::MenuItemMatcher()
                        .action("indicator.testplayer3.desktop")
                        .label("TestPlayer3")
                        .themed_icon("icon", {"testplayer"})
                        .string_attribute("x-canonical-type", "com.canonical.unity.media-player")
                    )
                    .item(mh::MenuItemMatcher()
                        .string_attribute("x-canonical-previous-action","indicator.previous.testplayer3.desktop")
                        .string_attribute("x-canonical-play-action","indicator.play.testplayer3.desktop")
                        .string_attribute("x-canonical-next-action","indicator.next.testplayer3.desktop")
                        .string_attribute("x-canonical-type","com.canonical.unity.playback-item")
                    )
                )
            .item(mh::MenuItemMatcher()
                            .label("Sound Settings…")
             )
        ).match());

    // start the rest of players
    EXPECT_TRUE(startTestMprisPlayer("testplayer1"));
    EXPECT_TRUE(startTestMprisPlayer("testplayer2"));

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
                .item(volumeSlider(INITIAL_VOLUME, "Volume"))
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
                        .section()
                        .item(mh::MenuItemMatcher()
                            .action("indicator.testplayer2.desktop")
                            .label("TestPlayer2")
                            .themed_icon("icon", {"testplayer"})
                            .string_attribute("x-canonical-type", "com.canonical.unity.media-player")
                        )
                        .item(mh::MenuItemMatcher()
                            .string_attribute("x-canonical-previous-action","indicator.previous.testplayer2.desktop")
                            .string_attribute("x-canonical-play-action","indicator.play.testplayer2.desktop")
                            .string_attribute("x-canonical-next-action","indicator.next.testplayer2.desktop")
                            .string_attribute("x-canonical-type","com.canonical.unity.playback-item")
                        )
                    )
            .item(mh::MenuItemMatcher()
                        .section()
                        .item(mh::MenuItemMatcher()
                            .action("indicator.testplayer3.desktop")
                            .label("TestPlayer3")
                            .themed_icon("icon", {"testplayer"})
                            .string_attribute("x-canonical-type", "com.canonical.unity.media-player")
                        )
                        .item(mh::MenuItemMatcher()
                            .string_attribute("x-canonical-previous-action","indicator.previous.testplayer3.desktop")
                            .string_attribute("x-canonical-play-action","indicator.play.testplayer3.desktop")
                            .string_attribute("x-canonical-next-action","indicator.next.testplayer3.desktop")
                            .string_attribute("x-canonical-type","com.canonical.unity.playback-item")
                        )
                    )
            .item(mh::MenuItemMatcher()
                            .label("Sound Settings…")
             )
        ).match());

    // stop all players
    EXPECT_TRUE(stopTestMprisPlayer("testplayer1"));
    EXPECT_TRUE(stopTestMprisPlayer("testplayer2"));
    EXPECT_TRUE(stopTestMprisPlayer("testplayer3"));

    // check that player 3 is the only one with playback controls
    // as it was the last one being stopped
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
                .item(volumeSlider(INITIAL_VOLUME, "Volume"))
            )
            .item(mh::MenuItemMatcher()
                    .section()
                    .item(mh::MenuItemMatcher()
                        .action("indicator.testplayer1.desktop")
                        .label("TestPlayer1")
                        .themed_icon("icon", {"testplayer"})
                        .string_attribute("x-canonical-type", "com.canonical.unity.media-player")
                    )
            )
            .item(mh::MenuItemMatcher()
                    .section()
                    .item(mh::MenuItemMatcher()
                        .action("indicator.testplayer2.desktop")
                        .label("TestPlayer2")
                        .themed_icon("icon", {"testplayer"})
                        .string_attribute("x-canonical-type", "com.canonical.unity.media-player")
                    )
                )
            .item(mh::MenuItemMatcher()
                    .section()
                    .item(mh::MenuItemMatcher()
                        .action("indicator.testplayer3.desktop")
                        .label("TestPlayer3")
                        .themed_icon("icon", {"testplayer"})
                        .string_attribute("x-canonical-type", "com.canonical.unity.media-player")
                    )
                    .item(mh::MenuItemMatcher()
                        .string_attribute("x-canonical-play-action","indicator.play.testplayer3.desktop")
                        .string_attribute("x-canonical-type","com.canonical.unity.playback-item")
                    )
                )
            .item(mh::MenuItemMatcher()
                            .label("Sound Settings…")
             )
        ).match());

    // check that the last running player is the one we expect
    QVariant lastPlayerRunning = waitLastRunningPlayerChanged();
    EXPECT_TRUE(lastPlayerRunning.type() == QVariant::String);
    EXPECT_EQ(lastPlayerRunning.toString(), "testplayer3.desktop");

    // restart the indicator to simulate a new user session
    ASSERT_NO_THROW(startIndicator());

    // check that player 3 is the only one with playback controls
    // as it was the last one being stopped
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
                .item(volumeSlider(INITIAL_VOLUME, "Volume"))
            )
            .item(mh::MenuItemMatcher()
                    .section()
                    .item(mh::MenuItemMatcher()
                        .action("indicator.testplayer3.desktop")
                        .label("TestPlayer3")
                        .themed_icon("icon", {"testplayer"})
                        .string_attribute("x-canonical-type", "com.canonical.unity.media-player")
                    )
                    .item(mh::MenuItemMatcher()
                        .string_attribute("x-canonical-play-action","indicator.play.testplayer3.desktop")
                        .string_attribute("x-canonical-type","com.canonical.unity.playback-item")
                    )
                )
            .item(mh::MenuItemMatcher()
                    .section()
                    .item(mh::MenuItemMatcher()
                        .action("indicator.testplayer1.desktop")
                        .label("TestPlayer1")
                        .themed_icon("icon", {"testplayer"})
                        .string_attribute("x-canonical-type", "com.canonical.unity.media-player")
                    )
            )
            .item(mh::MenuItemMatcher()
                    .section()
                    .item(mh::MenuItemMatcher()
                        .action("indicator.testplayer2.desktop")
                        .label("TestPlayer2")
                        .themed_icon("icon", {"testplayer"})
                        .string_attribute("x-canonical-type", "com.canonical.unity.media-player")
                    )
                )
            .item(mh::MenuItemMatcher()
                            .label("Sound Settings…")
             )
        ).match());

}

TEST_F(TestIndicator, DesktopMprisPlayerButtonsState)
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
                .item(volumeSlider(INITIAL_VOLUME, "Volume"))
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

    // change the state of CanGoNext
    EXPECT_TRUE(setTestMprisPlayerProperty("testplayer1", "CanGoNext", false));

    // verify that the action changes
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
                .item(volumeSlider(INITIAL_VOLUME, "Volume"))
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
                    .attribute_not_set("x-canonical-next-action")
                    .string_attribute("x-canonical-type","com.canonical.unity.playback-item")
                )
            )
            .item(mh::MenuItemMatcher()
                            .label("Sound Settings…")
             )
        ).match());


    // change the state of CanGoPrevious
    EXPECT_TRUE(setTestMprisPlayerProperty("testplayer1", "CanGoPrevious", false));

    // verify that the action changes
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
                .item(volumeSlider(INITIAL_VOLUME, "Volume"))
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
                    .attribute_not_set("x-canonical-previous-action")
                    .string_attribute("x-canonical-play-action","indicator.play.testplayer1.desktop")
                    .attribute_not_set("x-canonical-next-action")
                    .string_attribute("x-canonical-type","com.canonical.unity.playback-item")
                )
            )
            .item(mh::MenuItemMatcher()
                            .label("Sound Settings…")
             )
        ).match());

    // set back both to true
    EXPECT_TRUE(setTestMprisPlayerProperty("testplayer1", "CanGoNext", true));
    EXPECT_TRUE(setTestMprisPlayerProperty("testplayer1", "CanGoPrevious", true));

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
                .item(volumeSlider(INITIAL_VOLUME, "Volume"))
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

    EXPECT_TRUE(resetAllowAmplifiedVolume());
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

    // Generate a random volume in the range [0...0.33]
    QTime now = QTime::currentTime();
    qsrand(now.msec());
    int randInt = qrand() % 33;
    const double randomVolume = randInt / 100.0;

    // play a test sound, it should NOT change the role in the indicator
    EXPECT_TRUE(startTestSound("multimedia"));
    EXPECT_FALSE(waitVolumeChangedInIndicator());

    // set the random volume
    EXPECT_TRUE(setSinkVolume(randomVolume));
    if (randomVolume != INITIAL_VOLUME)
    {
        EXPECT_TRUE(waitVolumeChangedInIndicator());
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
                .item(volumeSlider(randomVolume, "Volume"))
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
                .item(volumeSlider(randomVolume, "Volume"))
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

TEST_F(TestIndicator, DISABLED_PhoneNotificationVolume)
{
    double INITIAL_VOLUME = 0.0;

    QSignalSpy notificationsSpy(&notificationsMockInterface(),
                               SIGNAL(MethodCalled(const QString &, const QVariantList &)));

    ASSERT_NO_THROW(startAccountsService());
    EXPECT_TRUE(clearGSettingsPlayers());
    ASSERT_NO_THROW(startPulsePhone());

    // initialize volumes in pulseaudio
    EXPECT_TRUE(setStreamRestoreVolume("alert", INITIAL_VOLUME));

    // start now the indicator, so it picks the new volumes
    ASSERT_NO_THROW(startIndicator());

    // check the initial state
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
                .item(volumeSlider(INITIAL_VOLUME, "Volume"))
            )
            .item(mh::MenuItemMatcher()
                .label("Sound Settings…")
                .action("indicator.phone-settings")
            )
        ).match());

    // change volume to 1.0
    setActionValue("volume", QVariant::fromValue(1.0));

    WAIT_FOR_SIGNALS(notificationsSpy, 3);

    // the first time we also have the calls to
    // GetServerInformation and GetCapabilities
    checkNotificationWithNoArgs("GetServerInformation", notificationsSpy.at(0));
    checkNotificationWithNoArgs("GetCapabilities", notificationsSpy.at(1));
    checkVolumeNotification(1.0, "Speakers", false, notificationsSpy.at(2));

    notificationsSpy.clear();
    setActionValue("volume", QVariant::fromValue(0.0));

    WAIT_FOR_SIGNALS(notificationsSpy, 2)

    checkNotificationWithNoArgs("GetCapabilities", notificationsSpy.at(0));
    checkVolumeNotification(0.0, "Speakers", false, notificationsSpy.at(1));

    notificationsSpy.clear();
    setActionValue("volume", QVariant::fromValue(0.5));

    WAIT_FOR_SIGNALS(notificationsSpy, 2)

    checkNotificationWithNoArgs("GetCapabilities", notificationsSpy.at(0));
    checkVolumeNotification(0.5, "Speakers", false, notificationsSpy.at(1));
}

TEST_F(TestIndicator, DISABLED_PhoneNotificationWarningVolume)
{
    double INITIAL_VOLUME = 0.0;

    QSignalSpy notificationsSpy(&notificationsMockInterface(),
                                   SIGNAL(MethodCalled(const QString &, const QVariantList &)));

    ASSERT_NO_THROW(startAccountsService());
    ASSERT_NO_THROW(startPulsePhone());

    // initialize volumes in pulseaudio
    EXPECT_TRUE(setStreamRestoreVolume("alert", INITIAL_VOLUME));
    EXPECT_TRUE(setStreamRestoreVolume("multimedia", INITIAL_VOLUME));

    // start now the indicator, so it picks the new volumes
    ASSERT_NO_THROW(startIndicator());

    // activate the headphones
    EXPECT_TRUE(activateHeadphones(true));

    // set an initial volume to the alert role
    setStreamRestoreVolume("alert", 1.0);
    EXPECT_TRUE(waitVolumeChangedInIndicator());

    // play a test sound, it should change the role in the indicator
    EXPECT_TRUE(startTestSound("multimedia"));
    EXPECT_TRUE(waitVolumeChangedInIndicator());

    WAIT_FOR_SIGNALS(notificationsSpy, 3);
    // the first time we also have the calls to
    // GetServerInformation and GetCapabilities
    checkNotificationWithNoArgs("GetServerInformation", notificationsSpy.at(0));
    checkNotificationWithNoArgs("GetCapabilities", notificationsSpy.at(1));
    checkVolumeNotification(0.0, "Headphones", false, notificationsSpy.at(2));
    notificationsSpy.clear();

    // change volume to 0.3... no warning should be emitted
    setActionValue("volume", QVariant::fromValue(0.3));
    EXPECT_TRUE(waitVolumeChangedInIndicator());

    WAIT_FOR_SIGNALS(notificationsSpy, 2);

    checkNotificationWithNoArgs("GetCapabilities", notificationsSpy.at(0));
    checkVolumeNotification(0.3, "Headphones", false, notificationsSpy.at(1));
    notificationsSpy.clear();

    // change volume to 0.5... no warning should be emitted
    setActionValue("volume", QVariant::fromValue(0.5));
    EXPECT_TRUE(waitVolumeChangedInIndicator());

    WAIT_FOR_SIGNALS(notificationsSpy, 2);

    checkNotificationWithNoArgs("GetCapabilities", notificationsSpy.at(0));
    checkVolumeNotification(0.5, "Headphones", false, notificationsSpy.at(1));
    notificationsSpy.clear();

    // change volume to 1.0... warning should be emitted
    setActionValue("volume", QVariant::fromValue(1.0));
    EXPECT_TRUE(waitVolumeChangedInIndicator());

    WAIT_FOR_SIGNALS(notificationsSpy, 8);

    // the notification is sent twice (TODO check why)
    checkNotificationWithNoArgs("GetCapabilities", notificationsSpy.at(0));
    checkCloseNotification(1, notificationsSpy.at(1));
    checkHighVolumeNotification(notificationsSpy.at(2));
    checkNotificationWithNoArgs("GetCapabilities", notificationsSpy.at(3));
    checkCloseNotification(1, notificationsSpy.at(4));
    checkHighVolumeNotification(notificationsSpy.at(5));

    // get the last notification ID
    int idNotification = getNotificationID(notificationsSpy.at(5));
    ASSERT_NE(-1, idNotification);

    // check the sync value before cancelling the dialog
    bool isValid;
    qlonglong syncValueBeforeCancel = getVolumeSyncValue(&isValid);
    EXPECT_TRUE(isValid);

    // cancel the dialog
    pressNotificationButton(idNotification, "cancel");

    // check that the volume was clamped
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
                .item(volumeSlider(0.74, "Volume (Headphones)"))
            )
        ).match());

    // verify that the sync value is increased
    qlonglong syncValueAfterCancel = getVolumeSyncValue(&isValid);
    EXPECT_TRUE(isValid);
    EXPECT_NE(syncValueBeforeCancel, syncValueAfterCancel);

    // try again...
    notificationsSpy.clear();

    // change volume to 1.0... warning should be emitted
    setActionValue("volume", QVariant::fromValue(1.0));
    EXPECT_TRUE(waitVolumeChangedInIndicator());

    WAIT_FOR_SIGNALS(notificationsSpy, 6);

    checkNotificationWithNoArgs("GetCapabilities", notificationsSpy.at(0));
    checkHighVolumeNotification(notificationsSpy.at(1));
    checkNotificationWithNoArgs("GetCapabilities", notificationsSpy.at(2));
    checkHighVolumeNotification(notificationsSpy.at(3));

    // get the last notification ID
    idNotification = getNotificationID(notificationsSpy.at(1));
    ASSERT_NE(-1, idNotification);

    // this time we approve
    pressNotificationButton(idNotification, "ok");

    // check that the volume was applied
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
                .item(volumeSlider(1.0, "Volume (Headphones)"))
                .item(mh::MenuItemMatcher()
                    .action("indicator.high-volume-warning-item")
                    .label("High volume can damage your hearing.")
                )
            )
        ).match());

    // after the warning was approved we should be able to modify the volume
    // and don't get the warning
    notificationsSpy.clear();

    // change volume to 0.5... no warning should be emitted
    setActionValue("volume", QVariant::fromValue(0.5));
    EXPECT_TRUE(waitVolumeChangedInIndicator());

    WAIT_FOR_SIGNALS(notificationsSpy, 6);

    // check the notification TODO check why the sound indicator sends it twice
    checkNotificationWithNoArgs("GetCapabilities", notificationsSpy.at(0));
    checkCloseNotification(idNotification, notificationsSpy.at(1));
    checkVolumeNotification(0.5, "Headphones", false, notificationsSpy.at(2));
    checkNotificationWithNoArgs("GetCapabilities", notificationsSpy.at(3));
    checkCloseNotification(idNotification, notificationsSpy.at(4));
    checkVolumeNotification(0.5, "Headphones", false, notificationsSpy.at(5));

    // check that the volume was applied
    // and that we don't have the warning item
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
                .item(volumeSlider(0.5, "Volume (Headphones)"))
            )
        ).match());

    // now set high volume again, we should not get the warning dialog
    // as we already approved it
    notificationsSpy.clear();

    setActionValue("volume", QVariant::fromValue(1.0));
    EXPECT_TRUE(waitVolumeChangedInIndicator());

    WAIT_FOR_SIGNALS(notificationsSpy, 4);

    // check the notification TODO check why the sound indicator sends it twice
    checkNotificationWithNoArgs("GetCapabilities", notificationsSpy.at(0));
    checkVolumeNotification(1.0, "Headphones", true, notificationsSpy.at(1));
    checkNotificationWithNoArgs("GetCapabilities", notificationsSpy.at(2));
    checkVolumeNotification(1.0, "Headphones", true, notificationsSpy.at(3));
}

TEST_F(TestIndicator, DISABLED_PhoneNotificationWarningVolumeAlertMode)
{
    double INITIAL_VOLUME = 0.0;

    QSignalSpy notificationsSpy(&notificationsMockInterface(),
                                   SIGNAL(MethodCalled(const QString &, const QVariantList &)));

    ASSERT_NO_THROW(startAccountsService());
    ASSERT_NO_THROW(startPulsePhone());

    // initialize volumes in pulseaudio
    EXPECT_TRUE(setStreamRestoreVolume("alert", INITIAL_VOLUME));
    EXPECT_TRUE(setStreamRestoreVolume("multimedia", INITIAL_VOLUME));

    // start now the indicator, so it picks the new volumes
    ASSERT_NO_THROW(startIndicator());

    // activate the headphones
    EXPECT_TRUE(activateHeadphones(true));

    // set an initial volume to the alert role
    setStreamRestoreVolume("alert", 1.0);
    EXPECT_TRUE(waitVolumeChangedInIndicator());

    // change volume to 0.0... no warning should be emitted
    setActionValue("volume", QVariant::fromValue(0.0));

    WAIT_FOR_SIGNALS(notificationsSpy, 5);

    // the first time we also have the calls to
    // GetServerInformation and GetCapabilities
    checkNotificationWithNoArgs("GetServerInformation", notificationsSpy.at(0));
    checkNotificationWithNoArgs("GetCapabilities", notificationsSpy.at(1));
    checkVolumeNotification(0.0, "Headphones", false, notificationsSpy.at(2));
    checkNotificationWithNoArgs("GetCapabilities", notificationsSpy.at(3));
    checkVolumeNotification(0.0, "Headphones", false, notificationsSpy.at(4));
    notificationsSpy.clear();

    // change volume to 0.5... no warning should be emitted
    setActionValue("volume", QVariant::fromValue(0.5));

    WAIT_FOR_SIGNALS(notificationsSpy, 2);

    checkNotificationWithNoArgs("GetCapabilities", notificationsSpy.at(0));
    checkVolumeNotification(0.5, "Headphones", false, notificationsSpy.at(1));
    notificationsSpy.clear();

    // change volume to 1.0... no warning should be emitted, we are in alert mode
    setActionValue("volume", QVariant::fromValue(1.0));

    WAIT_FOR_SIGNALS(notificationsSpy, 2);

    checkNotificationWithNoArgs("GetCapabilities", notificationsSpy.at(0));
    checkVolumeNotification(1.0, "Headphones", false, notificationsSpy.at(1));
    notificationsSpy.clear();
}

TEST_F(TestIndicator, DISABLED_PhoneNotificationHeadphoneSpeakerWiredLabels)
{
    checkPortDevicesLabels(WIRED, WIRED);
}

TEST_F(TestIndicator, DISABLED_PhoneNotificationHeadphoneSpeakerBluetoothLabels)
{
    checkPortDevicesLabels(BLUETOOTH, BLUETOOTH);
}

TEST_F(TestIndicator, DISABLED_PhoneNotificationHeadphoneSpeakerUSBLabels)
{
    checkPortDevicesLabels(USB, USB);
}

TEST_F(TestIndicator, DISABLED_PhoneNotificationHeadphoneSpeakerHDMILabels)
{
    checkPortDevicesLabels(HDMI, HDMI);
}

} // namespace
