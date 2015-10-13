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

#pragma once

#include <libqtdbustest/DBusTestRunner.h>
#include <libqtdbustest/QProcessDBusService.h>
#include <libqtdbusmock/DBusMock.h>

#include <unity/gmenuharness/MatchUtils.h>
#include <unity/gmenuharness/MenuMatcher.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

class MenusInterface;
class DBusPulseVolume;
class DBusPropertiesInterface;
class QSignalSpy;

#define WAIT_FOR_SIGNALS(signalSpy, signalsExpected)\
{\
    while (signalSpy.size() < signalsExpected)\
    {\
        ASSERT_TRUE(signalSpy.wait());\
    }\
    ASSERT_EQ(signalsExpected, signalSpy.size());\
}

class IndicatorSoundTestBase: public testing::Test
{
public:
    IndicatorSoundTestBase();

    ~IndicatorSoundTestBase();

protected:
    void SetUp() override;
    void TearDown() override;

    void startIndicator();
    void startPulseDesktop();
    void startPulsePhone();
    void startAccountsService();

    bool clearGSettingsPlayers();

    bool startTestMprisPlayer(QString const& playerName);

    bool setTestMprisPlayerProperty(QString const &testPlayer, QString const &property, bool value);

    bool setStreamRestoreVolume(QString const &role, double volume);

    bool setSinkVolume(double volume);

    bool startTestSound(QString const &role);

    void stopTestSound();

    static std::shared_ptr<GVariant> volume_variant(double volume);

    static unity::gmenuharness::MenuMatcher::Parameters desktopParameters();

    static unity::gmenuharness::MenuMatcher::Parameters phoneParameters();

    static unity::gmenuharness::MenuItemMatcher volumeSlider(double volume);

    static unity::gmenuharness::MenuItemMatcher silentModeSwitch(bool toggled);

    bool waitMenuChange();

    bool initializeMenuChangedSignal();

    bool waitVolumeChangedInIndicator();

    void initializeAccountsInterface();

    OrgFreedesktopDBusMockInterface& notificationsMockInterface();

    bool setActionValue(const QString & action, QVariant value);

    bool pressNotificationButton(int id, const QString & button);

    bool qDBusArgumentToMap(QVariant const& variant, QVariantMap& map);

    void checkVolumeNotification(double volume, QString const& label, bool isLoud, QVariantList call);

    void checkHighVolumeNotification(QVariantList call);

    void checkCloseNotification(int id, QVariantList call);

    void checkNotificationWithNoArgs(QString const& method, QVariantList call);

    int getNotificationID(QVariantList call);

    bool activateHeadphones(bool headphonesActive);

    QtDBusTest::DBusTestRunner dbusTestRunner;

    QtDBusMock::DBusMock dbusMock;

    QtDBusTest::DBusServicePtr indicator;

    QtDBusTest::DBusServicePtr pulseaudio;

    QtDBusTest::DBusServicePtr accountsService;

    QProcess testSoundProcess;

    QProcess testPlayer1;

    std::unique_ptr<MenusInterface> menu_interface_;

    std::unique_ptr<DBusPropertiesInterface> accounts_interface_;

    std::unique_ptr<QSignalSpy> signal_spy_volume_changed_;

    std::unique_ptr<QSignalSpy> signal_spy_menu_changed_;
};
