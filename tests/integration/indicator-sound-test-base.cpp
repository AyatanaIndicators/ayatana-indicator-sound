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

#include "indicator-sound-test-base.h"

#include "dbus_menus_interface.h"
#include "dbus_properties_interface.h"
#include "dbus_accounts_interface.h"
#include "dbus_accountssound_interface.h"
#include "dbus_notifications_interface.h"
#include "dbus-types.h"

#include <gio/gio.h>
#include <chrono>
#include <thread>

#include <QSignalSpy>
#include "utils/dbus-pulse-volume.h"

using namespace QtDBusTest;
using namespace QtDBusMock;
using namespace std;
using namespace testing;
namespace mh = unity::gmenuharness;

namespace
{
    const int MAX_TIME_WAITING_FOR_NOTIFICATIONS = 2000;
}

IndicatorSoundTestBase::IndicatorSoundTestBase() :
    dbusMock(dbusTestRunner)
{
}

IndicatorSoundTestBase::~IndicatorSoundTestBase()
{
}

void IndicatorSoundTestBase::SetUp()
{
    setenv("XDG_DATA_DIRS", XDG_DATA_DIRS, true);
    setenv("DBUS_SYSTEM_BUS_ADDRESS", dbusTestRunner.systemBus().toStdString().c_str(), true);
    setenv("DBUS_SESSION_BUS_ADDRESS", dbusTestRunner.sessionBus().toStdString().c_str(), true);
    dbusMock.registerNotificationDaemon();

    dbusTestRunner.startServices();

    auto& notifications = notificationsMockInterface();
    notifications.AddMethod("org.freedesktop.Notifications",
                            "GetCapabilities",
                            "",
                            "as",
                            "ret = ['actions', 'body', 'body-markup', 'icon-static', 'image/svg+xml', 'x-canonical-private-synchronous', 'x-canonical-append', 'x-canonical-private-icon-only', 'x-canonical-truncation', 'private-synchronous', 'append', 'private-icon-only', 'truncation']"
                         ).waitForFinished();

    int waitedTime = 0;
    while (!dbusTestRunner.sessionConnection().interface()->isServiceRegistered("org.freedesktop.Notifications") && waitedTime < MAX_TIME_WAITING_FOR_NOTIFICATIONS)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        waitedTime += 10;
    }
}

void IndicatorSoundTestBase::TearDown()
{
    unsetenv("XDG_DATA_DIRS");
    unsetenv("PULSE_SERVER");
    unsetenv("DBUS_SYSTEM_BUS_ADDRESS");
}

void gvariant_deleter(GVariant* varptr)
{
    if (varptr != nullptr)
    {
        g_variant_unref(varptr);
    }
}

std::shared_ptr<GVariant> IndicatorSoundTestBase::volume_variant(double volume)
{
    GVariantBuilder builder;

    g_variant_builder_init(&builder, G_VARIANT_TYPE_VARDICT);
    g_variant_builder_add(&builder,
                          "{sv}",
                          "title",
                          g_variant_new_string("_Sound"));

    g_variant_builder_add(&builder,
                          "{sv}",
                          "accessible-desc",
                          g_variant_new_string("_Sound"));

    auto icon = g_themed_icon_new("icon");
    g_variant_builder_add(&builder,
                          "{sv}",
                          "icon",
                          g_icon_serialize(icon));

    g_variant_builder_add(&builder,
                          "{sv}",
                          "visible",
                          g_variant_new_boolean(true));
    return shared_ptr<GVariant>(g_variant_builder_end(&builder), &gvariant_deleter);
}

bool IndicatorSoundTestBase::setStreamRestoreVolume(QString const &role, double volume)
{
    QProcess setVolume;
    setVolume.start(VOLUME_SET_BIN, QStringList()
                                        << role
                                        << QString("%1").arg(volume));
    if (!setVolume.waitForStarted())
        return false;

    if (!setVolume.waitForFinished())
        return false;

    return setVolume.exitCode() == 0;
}

bool IndicatorSoundTestBase::setSinkVolume(double volume)
{
    QString volume_percentage = QString("%1\%").arg(volume*100);
    QProcess setVolume;
    setVolume.start("pactl", QStringList()
                                        << "-s"
                                        << "127.0.0.1"
                                        << "set-sink-volume"
                                        << "0"
                                        << volume_percentage);
    if (!setVolume.waitForStarted())
        return false;

    if (!setVolume.waitForFinished())
        return false;

    return setVolume.exitCode() == 0;
}

bool IndicatorSoundTestBase::clearGSettingsPlayers()
{
    QProcess clearPlayers;

    clearPlayers.start("gsettings", QStringList()
                                        << "set"
                                        << "com.canonical.indicator.sound"
                                        << "interested-media-players"
                                        << "[]");

    return runProcess(clearPlayers);
}

bool IndicatorSoundTestBase::resetAllowAmplifiedVolume()
{
    QProcess proc;

    proc.start("gsettings", QStringList()
                                << "reset"
                                << "com.ubuntu.sound"
                                << "allow-amplified-volume");

    return runProcess(proc);
}

bool IndicatorSoundTestBase::runProcess(QProcess& proc)
{
    if (!proc.waitForStarted())
        return false;

    if (!proc.waitForFinished())
        return false;

    return proc.exitCode() == 0;
}

bool IndicatorSoundTestBase::startTestMprisPlayer(QString const& playerName)
{
    testPlayer1.terminate();
    testPlayer1.start(MEDIA_PLAYER_MPRIS_BIN, QStringList()
                                        << playerName);
    if (!testPlayer1.waitForStarted())
        return false;


    return true;
}

bool IndicatorSoundTestBase::setTestMprisPlayerProperty(QString const &testPlayer, QString const &property, bool value)
{
    QProcess setProperty;
    QString strValue;
    strValue = value ? "true" : "false";

    setProperty.start(MEDIA_PLAYER_MPRIS_UPDATE_BIN, QStringList()
                                        << testPlayer
                                        << property
                                        << strValue);
    if (!setProperty.waitForStarted())
        return false;

    if (!setProperty.waitForFinished())
        return false;

    return setProperty.exitCode() == 0;
}

bool IndicatorSoundTestBase::startTestSound(QString const &role)
{
    testSoundProcess.terminate();
    testSoundProcess.start("paplay", QStringList()
                                << "-s"
                                << "127.0.0.1"
                                << TEST_SOUND
                                << QString("--property=media.role=%1").arg(role));

    if (!testSoundProcess.waitForStarted())
        return false;

    return true;
}

void IndicatorSoundTestBase::stopTestSound()
{
    testSoundProcess.terminate();
}

void IndicatorSoundTestBase::startPulseDesktop(DevicePortType speakerPort, DevicePortType headphonesPort)
{
    try
    {
        pulseaudio.reset(
                new QProcessDBusService(DBusTypes::DBUS_PULSE,
                                        QDBusConnection::SessionBus,
                                        "pulseaudio",
                                        QStringList() << "--start"
                                                      << "-vvvv"
                                                      << "--disable-shm=true"
                                                      << "--daemonize=false"
                                                      << "--use-pid-file=false"
                                                      << "--system=false"
                                                      << "--exit-idle-time=-1"
                                                      << "-n"
                                                      << QString("--load=module-null-sink sink_name=indicator_sound_test_speaker sink_properties=device.bus=%1").arg(getDevicePortString(speakerPort))
                                                      << QString("--load=module-null-sink sink_name=indicator_sound_test_headphones sink_properties=device.bus=%1").arg(getDevicePortString(headphonesPort))
                                                      << "--log-target=file:/tmp/pulse-daemon.log"
                                                      << "--load=module-dbus-protocol"
                                                      << "--load=module-native-protocol-tcp auth-ip-acl=127.0.0.1"
                ));
        pulseaudio->start(dbusTestRunner.sessionConnection());
    }
    catch (exception const& e)
    {
        cout << "pulseaudio(): " << e.what() << endl;
        throw;
    }
}

void IndicatorSoundTestBase::startPulsePhone(DevicePortType speakerPort, DevicePortType headphonesPort)
{
    try
    {
        pulseaudio.reset(
                new QProcessDBusService(DBusTypes::DBUS_PULSE,
                                        QDBusConnection::SessionBus,
                                        "pulseaudio",
                                        QStringList() << "--start"
                                                      << "-vvvv"
                                                      << "--disable-shm=true"
                                                      << "--daemonize=false"
                                                      << "--use-pid-file=false"
                                                      << "--system=false"
                                                      << "--exit-idle-time=-1"
                                                      << "-n"
                                                      << QString("--load=module-null-sink sink_name=indicator_sound_test_speaker sink_properties=device.bus=%1").arg(getDevicePortString(speakerPort))
                                                      << QString("--load=module-null-sink sink_name=indicator_sound_test_headphones sink_properties=device.bus=%1").arg(getDevicePortString(headphonesPort))
                                                      << "--log-target=file:/tmp/pulse-daemon.log"
                                                      << QString("--load=module-stream-restore restore_device=false restore_muted=false fallback_table=%1").arg(STREAM_RESTORE_TABLE)
                                                      << "--load=module-dbus-protocol"
                                                      << "--load=module-native-protocol-tcp auth-ip-acl=127.0.0.1"
                ));
        pulseaudio->start(dbusTestRunner.sessionConnection());
    }
    catch (exception const& e)
    {
        cout << "pulseaudio(): " << e.what() << endl;
        throw;
    }
}

void IndicatorSoundTestBase::startAccountsService()
{
    try
    {
        accountsService.reset(
                new QProcessDBusService(DBusTypes::ACCOUNTS_SERVICE,
                                        QDBusConnection::SystemBus,
                                        ACCOUNTS_SERVICE_BIN,
                                        QStringList()));
        accountsService->start(dbusTestRunner.systemConnection());

        initializeAccountsInterface();
    }
    catch (exception const& e)
    {
        cout << "accountsService(): " << e.what() << endl;
        throw;
    }
}

void IndicatorSoundTestBase::startIndicator()
{
    try
    {
        setenv("PULSE_SERVER", "127.0.0.1", true);
        indicator.reset(
                new QProcessDBusService(DBusTypes::DBUS_NAME,
                                        QDBusConnection::SessionBus,
                                        SOUND_SERVICE_BIN,
                                        QStringList()));
        indicator->start(dbusTestRunner.sessionConnection());
    }
    catch (exception const& e)
    {
        cout << "startIndicator(): " << e.what() << endl;
        throw;
    }
}

mh::MenuMatcher::Parameters IndicatorSoundTestBase::desktopParameters()
{
    return mh::MenuMatcher::Parameters(
            "com.canonical.indicator.sound",
            { { "indicator", "/com/canonical/indicator/sound" } },
            "/com/canonical/indicator/sound/desktop");
}

mh::MenuMatcher::Parameters IndicatorSoundTestBase::phoneParameters()
{
    return mh::MenuMatcher::Parameters(
            "com.canonical.indicator.sound",
            { { "indicator", "/com/canonical/indicator/sound" } },
            "/com/canonical/indicator/sound/phone");
}

unity::gmenuharness::MenuItemMatcher IndicatorSoundTestBase::volumeSlider(double volume, QString const &label)
{
    return mh::MenuItemMatcher().radio()
            .label(label.toStdString())
            .round_doubles(0.1)
            .int32_attribute("target", 0)
            .double_attribute("min-value", 0.0)
            .double_attribute("max-value", 1.0)
            .double_attribute("step", 0.01)
            .string_attribute("x-canonical-type", "com.canonical.unity.slider")
            .themed_icon("max-icon", {"audio-volume-high-panel", "audio-volume-high", "audio-volume", "audio"})
            .themed_icon("min-icon", {"audio-volume-low-zero-panel", "audio-volume-low-zero", "audio-volume-low", "audio-volume", "audio"})
            .pass_through_double_attribute("action", volume);
}

unity::gmenuharness::MenuItemMatcher IndicatorSoundTestBase::silentModeSwitch(bool toggled)
{
    return mh::MenuItemMatcher::checkbox()
        .label("Silent Mode")
        .action("indicator.silent-mode")
        .toggled(toggled);
}

bool IndicatorSoundTestBase::waitMenuChange()
{
    if (signal_spy_menu_changed_)
    {
        return signal_spy_menu_changed_->wait();
    }
    return false;
}

bool IndicatorSoundTestBase::initializeMenuChangedSignal()
{
    if (!menu_interface_)
    {
        menu_interface_.reset(new MenusInterface("com.canonical.indicator.sound",
                                                 "/com/canonical/indicator/sound",
                                                 dbusTestRunner.sessionConnection(), 0));
    }
    if (menu_interface_)
    {
        qDebug() << "Waiting for signal";
        signal_spy_menu_changed_.reset(new QSignalSpy(menu_interface_.get(), &MenusInterface::Changed));
    }
    if (!menu_interface_ || !signal_spy_menu_changed_)
    {
        return false;
    }
    return true;
}

bool IndicatorSoundTestBase::waitVolumeChangedInIndicator()
{
    if (signal_spy_volume_changed_)
    {
        return signal_spy_volume_changed_->wait();
    }
    return false;
}

void IndicatorSoundTestBase::initializeAccountsInterface()
{
    auto username = qgetenv("USER");
    if (username != "")
    {
        main_accounts_interface_.reset(new AccountsInterface("org.freedesktop.Accounts",
                                                        "/org/freedesktop/Accounts",
                                                        dbusTestRunner.systemConnection(), 0));

        QDBusReply<QDBusObjectPath> userResp = main_accounts_interface_->call(QLatin1String("FindUserByName"),
                                                                  QLatin1String(username));

        if (!userResp.isValid())
        {
            qWarning() << "SetVolume::initializeAccountsInterface(): D-Bus error: " << userResp.error().message();
        }

        auto userPath = userResp.value().path();
        if (userPath != "")
        {
            std::unique_ptr<AccountsSoundInterface> soundInterface(new AccountsSoundInterface("org.freedesktop.Accounts",
                                                                    userPath,
                                                                    dbusTestRunner.systemConnection(), 0));

            accounts_interface_.reset(new DBusPropertiesInterface("org.freedesktop.Accounts",
                                                                userPath,
                                                                dbusTestRunner.systemConnection(), 0));
            if (!accounts_interface_->isValid())
            {
                qWarning() << "SetVolume::initializeAccountsInterface(): D-Bus error: " << accounts_interface_->lastError().message();
            }
            signal_spy_volume_changed_.reset(new QSignalSpy(accounts_interface_.get(),&DBusPropertiesInterface::PropertiesChanged));
        }
    }
}

OrgFreedesktopDBusMockInterface& IndicatorSoundTestBase::notificationsMockInterface()
{
    return dbusMock.mockInterface("org.freedesktop.Notifications",
                                   "/org/freedesktop/Notifications",
                                   "org.freedesktop.Notifications",
                                   QDBusConnection::SessionBus);
}

bool IndicatorSoundTestBase::setActionValue(const QString & action, QVariant value)
{
    QDBusInterface actionsInterface(DBusTypes::DBUS_NAME,
                                    DBusTypes::MAIN_SERVICE_PATH,
                                    DBusTypes::ACTIONS_INTERFACE,
                                    dbusTestRunner.sessionConnection());

    QDBusVariant dbusVar(value);
    auto resp = actionsInterface.call("SetState",
                                      action,
                                      QVariant::fromValue(dbusVar),
                                      QVariant::fromValue(QVariantMap()));

    if (resp.type() == QDBusMessage::ErrorMessage)
    {
        qCritical() << "IndicatorSoundTestBase::setActionValue(): Failed to set value for action "
                    << action
                    << " "
                    << resp.errorMessage();
        return false;
    }
    else
    {
        return true;
    }
}

bool IndicatorSoundTestBase::pressNotificationButton(int id, const QString & button)
{
    OrgFreedesktopDBusMockInterface actionsInterface("org.freedesktop.Notifications",
                                                     "/org/freedesktop/Notifications",
                                                     dbusTestRunner.sessionConnection());

    actionsInterface.EmitSignal(
            "org.freedesktop.Notifications",
                            "ActionInvoked", "us", QVariantList() << id << button);

    return true;
}

bool IndicatorSoundTestBase::qDBusArgumentToMap(QVariant const& variant, QVariantMap& map)
{
    if (variant.canConvert<QDBusArgument>())
    {
        QDBusArgument value(variant.value<QDBusArgument>());
        if (value.currentType() == QDBusArgument::MapType)
        {
            value >> map;
            return true;
        }
    }
    return false;
}

void IndicatorSoundTestBase::checkVolumeNotification(double volume, QString const& label, bool isLoud, QVariantList call)
{
    QString icon;
    if (volume <= 0.0)
    {
        icon = "audio-volume-muted";
    }
    else if (volume <= 0.3)
    {
        icon = "audio-volume-low";
    }
    else if (volume <= 0.7)
    {
        icon = "audio-volume-medium";
    }
    else
    {
        icon = "audio-volume-high";
    }

    ASSERT_NE(call.size(), 0);
    EXPECT_EQ("Notify", call.at(0));

    QVariantList const& args(call.at(1).toList());
    ASSERT_EQ(8, args.size());
    EXPECT_EQ("indicator-sound", args.at(0));
    EXPECT_EQ(icon, args.at(2));
    EXPECT_EQ("Volume", args.at(3));
    EXPECT_EQ(label, args.at(4));
    EXPECT_EQ(QStringList(), args.at(5));

    QVariantMap hints;
    ASSERT_TRUE(qDBusArgumentToMap(args.at(6), hints));
    ASSERT_TRUE(hints.contains("value"));
    ASSERT_TRUE(hints.contains("x-canonical-non-shaped-icon"));
    ASSERT_TRUE(hints.contains("x-canonical-value-bar-tint"));
    ASSERT_TRUE(hints.contains("x-canonical-private-synchronous"));

    EXPECT_EQ(volume*100, hints["value"]);
    EXPECT_EQ(true, hints["x-canonical-non-shaped-icon"]);
    EXPECT_EQ(isLoud, hints["x-canonical-value-bar-tint"]);
    EXPECT_EQ(true, hints["x-canonical-private-synchronous"]);
}

void IndicatorSoundTestBase::checkHighVolumeNotification(QVariantList call)
{
    ASSERT_NE(call.size(), 0);
    EXPECT_EQ("Notify", call.at(0));

    QVariantList const& args(call.at(1).toList());
    ASSERT_EQ(8, args.size());
    EXPECT_EQ("indicator-sound", args.at(0));
    EXPECT_EQ("Volume", args.at(3));
}

void IndicatorSoundTestBase::checkCloseNotification(int id, QVariantList call)
{
    EXPECT_EQ("CloseNotification", call.at(0));
    QVariantList const& args(call.at(1).toList());
    ASSERT_EQ(1, args.size());
}

void IndicatorSoundTestBase::checkNotificationWithNoArgs(QString const& method, QVariantList call)
{
    EXPECT_EQ(method, call.at(0));
    QVariantList const& args(call.at(1).toList());
    ASSERT_EQ(0, args.size());
}

int IndicatorSoundTestBase::getNotificationID(QVariantList call)
{
    if (call.size() == 0)
    {
        return -1;
    }
    QVariantList const& args(call.at(1).toList());
    if (args.size() != 8)
    {
        return -1;
    }
    if (args.at(0) != "indicator-sound")
    {
        return -1;
    }

    bool isInt;
    int id = args.at(1).toInt(&isInt);
    if (!isInt)
    {
        return -1;
    }
    return id;
}

bool IndicatorSoundTestBase::activateHeadphones(bool headphonesActive)
{
    QProcess pacltProcess;

    QString defaultSinkName = "indicator_sound_test_speaker";
    QString suspendedSinkName = "indicator_sound_test_headphones";
    if (headphonesActive)
    {
        defaultSinkName = "indicator_sound_test_headphones";
        suspendedSinkName = "indicator_sound_test_speaker";
    }

    pacltProcess.start("pactl", QStringList() << "-s"
                                              << "127.0.0.1"
                                              << "set-default-sink"
                                              << defaultSinkName);
    if (!pacltProcess.waitForStarted())
        return false;

    if (!pacltProcess.waitForFinished())
        return false;

    pacltProcess.start("pactl", QStringList() << "-s"
                                              << "127.0.0.1"
                                              << "suspend-sink"
                                              << defaultSinkName
                                              << "0");
    if (!pacltProcess.waitForStarted())
        return false;

    if (!pacltProcess.waitForFinished())
        return false;

    pacltProcess.start("pactl", QStringList() << "-s"
                                              << "127.0.0.1"
                                              << "suspend-sink"
                                              << suspendedSinkName
                                              << "1");
    if (!pacltProcess.waitForStarted())
        return false;

    if (!pacltProcess.waitForFinished())
        return false;

    return pacltProcess.exitCode() == 0;
}

QString IndicatorSoundTestBase::getDevicePortString(DevicePortType port)
{
    QString portString;

    switch (port)
    {
    case WIRED:
        portString = "wired";
        break;
    case BLUETOOTH:
        portString = "bluetooth";
        break;
    case USB:
        portString = "usb";
        break;
    case HDMI:
        portString = "hdmi";
        break;
    default:
        portString = "not_defined";
        break;
    }

    return portString;
}

void IndicatorSoundTestBase::checkPortDevicesLabels(DevicePortType speakerPort, DevicePortType headphonesPort)
{
    double INITIAL_VOLUME = 0.0;

    QString speakerString;
    QString speakerStringMenu;
    switch(speakerPort)
    {
    case WIRED:
        speakerString = "Speakers";
        speakerStringMenu = "Volume";
        break;
    case BLUETOOTH:
        speakerString = "Bluetooth speaker";
        speakerStringMenu = "Volume (Bluetooth)";
        break;
    case USB:
        speakerString = "Usb speaker";
        speakerStringMenu = "Volume (Usb)";
        break;
    case HDMI:
        speakerString = "HDMI speaker";
        speakerStringMenu = "Volume (HDMI)";
        break;
    }

    QString headphonesString;
    QString headphonesStringMenu;
    switch(headphonesPort)
    {
    case WIRED:
        headphonesString = "Headphones";
        headphonesStringMenu = "Volume (Headphones)";
        break;
    case BLUETOOTH:
        headphonesString = "Bluetooth headphones";
        headphonesStringMenu = "Volume (Bluetooth headphones)";
        break;
    case USB:
        headphonesString = "Usb headphones";
        headphonesStringMenu = "Volume (Usb headphones)";
        break;
    case HDMI:
        headphonesString = "HDMI headphones";
        headphonesStringMenu = "Volume (HDMI headphones)";
        break;
    }

    QSignalSpy notificationsSpy(&notificationsMockInterface(),
                                   SIGNAL(MethodCalled(const QString &, const QVariantList &)));

    ASSERT_NO_THROW(startAccountsService());
    ASSERT_NO_THROW(startPulsePhone(speakerPort, headphonesPort));

    // initialize volumes in pulseaudio
    EXPECT_TRUE(setStreamRestoreVolume("alert", INITIAL_VOLUME));
    EXPECT_TRUE(setStreamRestoreVolume("multimedia", INITIAL_VOLUME));

    // start now the indicator, so it picks the new volumes
    ASSERT_NO_THROW(startIndicator());

    // if the speaker is the normal one it does not emit any notification, as that's
    // the default one.
    // for the rest it notifies the output
    if (speakerPort != WIRED)
    {
        WAIT_FOR_SIGNALS(notificationsSpy, 3);

        // the first time we also have the calls to
        // GetServerInformation and GetCapabilities
        checkNotificationWithNoArgs("GetServerInformation", notificationsSpy.at(0));
        checkNotificationWithNoArgs("GetCapabilities", notificationsSpy.at(1));
        checkVolumeNotification(INITIAL_VOLUME, speakerString, false, notificationsSpy.at(2));
        notificationsSpy.clear();
    }

    notificationsSpy.clear();
    // activate the headphones
    EXPECT_TRUE(activateHeadphones(true));

    if (speakerPort == WIRED)
    {
        WAIT_FOR_SIGNALS(notificationsSpy, 3);

        // the first time we also have the calls to
        // GetServerInformation and GetCapabilities
        checkNotificationWithNoArgs("GetServerInformation", notificationsSpy.at(0));
        checkNotificationWithNoArgs("GetCapabilities", notificationsSpy.at(1));
        checkVolumeNotification(INITIAL_VOLUME, headphonesString, false, notificationsSpy.at(2));
        notificationsSpy.clear();
    }
    else
    {
        WAIT_FOR_SIGNALS(notificationsSpy, 2);
        checkNotificationWithNoArgs("GetCapabilities", notificationsSpy.at(0));
        checkVolumeNotification(INITIAL_VOLUME, headphonesString, false, notificationsSpy.at(1));
        notificationsSpy.clear();
    }

    // check the label in the menu
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
                .item(volumeSlider(INITIAL_VOLUME, headphonesStringMenu))
            )
        ).match());

    // deactivate the headphones
    EXPECT_TRUE(activateHeadphones(false));

    WAIT_FOR_SIGNALS(notificationsSpy, 2);
    checkNotificationWithNoArgs("GetCapabilities", notificationsSpy.at(0));
    checkVolumeNotification(INITIAL_VOLUME, speakerString, false, notificationsSpy.at(1));
    notificationsSpy.clear();

    // check the label in the menu
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
                .item(volumeSlider(INITIAL_VOLUME, speakerStringMenu))
            )
        ).match());
}

bool IndicatorSoundTestBase::setVolumeUntilAccountsIsConnected(double volume)
{
    int RETRY_TIME = 5000;

    setActionValue("volume", QVariant::fromValue(volume));
    while(!signal_spy_volume_changed_->wait(10) && RETRY_TIME)
    {
        RETRY_TIME -= 10;
        setActionValue("volume", QVariant::fromValue(volume));
    }
    return (signal_spy_volume_changed_->count() != 0);
}

QVariantList IndicatorSoundTestBase::getActionValue(QString const &action)
{
    if (!menu_interface_)
    {
        menu_interface_.reset(new MenusInterface("com.canonical.indicator.sound",
                                                 "/com/canonical/indicator/sound",
                                                 dbusTestRunner.sessionConnection(), 0));
    }
    if (menu_interface_)
    {
        QDBusReply<DBusActionResult> resp = menu_interface_->call(QLatin1String("Describe"),
                                                                  action);
        if (!resp.isValid())
        {
            qWarning() << "IndicatorSoundTestBase::getActionValue(): D-Bus error: " << resp.error().message();
            return QVariantList();
        }
        else
        {
            return resp.value().getValue();
        }
    }

    return QVariantList();
}

qlonglong IndicatorSoundTestBase::getVolumeSyncValue(bool *isValid)
{
    qlonglong result = 0;

    QVariantList varList = getActionValue("volume-sync");
    if (varList.size() == 1)
    {
        result = varList.at(0).toULongLong(isValid);
    }
    else
    {
        if (isValid)
        {
            *isValid = false;
        }
    }

    return result;
}

float IndicatorSoundTestBase::getVolumeValue(bool *isValid)
{
    float result = 0.0;

    QVariantList varList = getActionValue("volume");
    if (varList.size() == 1)
    {
        result = varList.at(0).toFloat(isValid);
    }
    else
    {
        if (isValid)
        {
            *isValid = false;
        }
    }

    return result;
}
