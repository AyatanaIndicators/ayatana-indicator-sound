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

TEST_F(TestIndicator, ChangeRoleVolume)
{
    double INITIAL_VOLUME = 0.0;

    ASSERT_NO_THROW(startPulse());

    // initialize volumes in pulseaudio
    EXPECT_TRUE(setVolume("mutimedia", INITIAL_VOLUME));
    EXPECT_TRUE(setVolume("alert", INITIAL_VOLUME));

    // start now the indicator, so it picks the new volumes
    ASSERT_NO_THROW(startIndicator());

    // Generate a random volume
    QTime now = QTime::currentTime();
    qsrand(now.msec());
    int randInt = qrand() % 100;
    double randomVolume = randInt / 100.0;

    // set an initial volume to the alert role
    setVolume("alert", 1.0);
    EXPECT_TRUE(waitVolumeChangedInIndicator());

    // play a test sound, it should change the role in the indicator
    EXPECT_TRUE(startTestSound("multimedia"));
    EXPECT_TRUE(waitVolumeChangedInIndicator());

    // set the random volume to the multimedia role
    EXPECT_TRUE(setVolume("multimedia", randomVolume));
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
    if (randomVolume != 1.0)
    {
        // we only wait if the volume in the alert and the
        // one set in the multimedia roles differ
        EXPECT_TRUE(waitVolumeChangedInIndicator());
    }

    // check the initial volume for the alert role
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
                .item(volumeSlider(1.0))
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
}

TEST_F(TestIndicator, BasicInitialVolume)
{
    double INITIAL_VOLUME = 0.0;

    ASSERT_NO_THROW(startPulse());

    // initialize volumes in pulseaudio
    EXPECT_TRUE(setVolume("alert", INITIAL_VOLUME));

    // start now the indicator, so it picks the new volumes
    ASSERT_NO_THROW(startIndicator());

    // check the initial volume for the alert role
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
                .item(volumeSlider(INITIAL_VOLUME))
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
}

} // namespace
