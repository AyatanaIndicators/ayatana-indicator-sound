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
class AccountsInterface;
class QSignalSpy;

#define WAIT_FOR_SIGNALS(signalSpy, signalsExpected)\
{\
    while (signalSpy.size() < signalsExpected)\
    {\
        ASSERT_TRUE(signalSpy.wait());\
    }\
    ASSERT_EQ(signalsExpected, signalSpy.size());\
}

#define WAIT_AT_LEAST_SIGNALS(signalSpy, signalsExpected)\
{\
    while (signalSpy.size() < signalsExpected)\
    {\
        ASSERT_TRUE(signalSpy.wait());\
    }\
    ASSERT_TRUE(signalsExpected <= signalSpy.size());\
}

class IndicatorSoundTestBase: public testing::Test
{
public:
    IndicatorSoundTestBase();

    ~IndicatorSoundTestBase();

    enum DevicePortType
    {
        WIRED,
        BLUETOOTH,
        USB,
        HDMI
    };

protected:
    void SetUp() override;
    void TearDown() override;

    void startIndicator();
    void startPulseDesktop(DevicePortType speakerPort=WIRED, DevicePortType headphonesPort=WIRED);
    void startPulsePhone(DevicePortType speakerPort=WIRED, DevicePortType headphonesPort=WIRED);
    void startAccountsService();

    bool clearGSettingsPlayers();
    bool resetAllowAmplifiedVolume();
    bool runProcess(QProcess&);

    bool startTestMprisPlayer(QString const& playerName);
    bool stopTestMprisPlayer(QString const& playerName);
    int findRunningTestMprisPlayer(QString const& playerName);

    bool setTestMprisPlayerProperty(QString const &testPlayer, QString const &property, bool value);

    bool setStreamRestoreVolume(QString const &role, double volume);

    bool setSinkVolume(double volume);

    bool startTestSound(QString const &role);

    void stopTestSound();

    static std::shared_ptr<GVariant> volume_variant(double volume);

    static unity::gmenuharness::MenuMatcher::Parameters desktopParameters();

    static unity::gmenuharness::MenuMatcher::Parameters phoneParameters();

    static unity::gmenuharness::MenuItemMatcher volumeSlider(double volume, QString const &label);

    static unity::gmenuharness::MenuItemMatcher micSlider(double volume, QString const &label);

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

    bool plugExternalMic(bool activate);

    bool setDefaultSinkOrSource(bool runForSinks, const QString & active, const QStringList & inactive);

    QString getDevicePortString(DevicePortType port);

    void checkPortDevicesLabels(DevicePortType speakerPort, DevicePortType headphonesPort);

    bool setVolumeUntilAccountsIsConnected(double volume);

    QVariantList getActionValue(QString const &action);

    qlonglong getVolumeSyncValue(bool *isValid = nullptr);

    float getVolumeValue(bool *isValid = nullptr);

    static QVariant waitPropertyChanged(QSignalSpy * signalSpy, QString property);

    QVariant waitLastRunningPlayerChanged();

    QtDBusTest::DBusTestRunner dbusTestRunner;

    QtDBusMock::DBusMock dbusMock;

    QtDBusTest::DBusServicePtr indicator;

    QtDBusTest::DBusServicePtr pulseaudio;

    QtDBusTest::DBusServicePtr accountsService;

    QProcess testSoundProcess;

    QProcess testPlayer1;

    struct TestPlayer
    {
        std::shared_ptr<QProcess> process;
        QString name;
    };
    QVector<TestPlayer> testPlayers;

    std::unique_ptr<MenusInterface> menu_interface_;

    std::unique_ptr<DBusPropertiesInterface> accounts_interface_;

    std::unique_ptr<AccountsInterface> main_accounts_interface_;

    std::unique_ptr<QSignalSpy> signal_spy_volume_changed_;

    std::unique_ptr<QSignalSpy> signal_spy_menu_changed_;
};
