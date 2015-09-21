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
#include "dbus-types.h"

#include <gio/gio.h>

#include <QSignalSpy>
#include "utils/dbus-pulse-volume.h"

using namespace QtDBusTest;
using namespace QtDBusMock;
using namespace std;
using namespace testing;
namespace mh = unity::gmenuharness;

IndicatorSoundTestBase::IndicatorSoundTestBase() :
    dbusMock(dbusTestRunner)
{
}

IndicatorSoundTestBase::~IndicatorSoundTestBase()
{

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

//    sleep(1);
    return true;
}

void IndicatorSoundTestBase::stopTestSound()
{
    testSoundProcess.terminate();
}

void IndicatorSoundTestBase::startPulseDesktop()
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
                                                      << "--load=module-null-sink"
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

void IndicatorSoundTestBase::startPulsePhone()
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
                                                      << "--load=module-null-sink"
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
                                        QStringList() ));
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

void IndicatorSoundTestBase::SetUp()
{
    setenv("GSETTINGS_SCHEMA_DIR", SCHEMA_DIR, true);
    setenv("GSETTINGS_BACKEND", "memory", true);
    setenv("DBUS_SYSTEM_BUS_ADDRESS", dbusTestRunner.systemBus().toStdString().c_str(), true);
}

void IndicatorSoundTestBase::TearDown()
{
    unsetenv("GSETTINGS_SCHEMA_DIR");
    unsetenv("GSETTINGS_BACKEND");
    unsetenv("PULSE_SERVER");
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

unity::gmenuharness::MenuItemMatcher IndicatorSoundTestBase::volumeSlider(double volume)
{
    return mh::MenuItemMatcher().radio()
            .label("Volume")
            .round_doubles(0.1)
            .int32_attribute("target", 0)
            .double_attribute("min-value", 0.0)
            .double_attribute("max-value", 1.0)
            .double_attribute("step", 0.01)
            .string_attribute("x-canonical-type", "com.canonical.unity.slider")
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
    if (!menu_interface_)
    {
        menu_interface_.reset(new MenusInterface("com.canonical.indicator.sound",
                                                 "/com/canonical/indicator/sound/desktop",
                                                QDBusConnection::sessionBus(), 0));
    }
    if (menu_interface_)
    {
        qDebug() << "Waiting for signal";
        QSignalSpy spy(menu_interface_.get(), &MenusInterface::Changed);
        qDebug() << "Signal count " << spy.count();
        return spy.wait();
    }
    return false;
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
        std::unique_ptr<AccountsInterface> setInterface(new AccountsInterface("org.freedesktop.Accounts",
                                                        "/org/freedesktop/Accounts",
                                                        QDBusConnection::systemBus(), 0));

        QDBusReply<QDBusObjectPath> userResp = setInterface->call(QLatin1String("FindUserByName"),
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
                                                                    QDBusConnection::systemBus(), 0));

            accounts_interface_.reset(new DBusPropertiesInterface("org.freedesktop.Accounts",
                                                                userPath,
                                                                soundInterface->connection(), 0));
            if (!accounts_interface_->isValid())
            {
                qWarning() << "SetVolume::initializeAccountsInterface(): D-Bus error: " << accounts_interface_->lastError().message();
            }
            signal_spy_volume_changed_.reset(new QSignalSpy(accounts_interface_.get(),&DBusPropertiesInterface::PropertiesChanged));
        }
    }
}
