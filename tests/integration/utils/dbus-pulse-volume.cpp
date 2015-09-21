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

#include "dbus-pulse-volume.h"

#include "dbus_properties_interface.h"
#include "dbus_accounts_interface.h"
#include "dbus_accountssound_interface.h"
#include "stream_restore_interface.h"

#include <pulse/volume.h>

#include <QSignalSpy>

unsigned int volumeDoubleToUint(double volume)
{
    double tmp = (double)(PA_VOLUME_NORM - PA_VOLUME_MUTED) * volume;
    return (unsigned int)tmp + PA_VOLUME_MUTED;
}

double volumeUIntToDoulbe(uint volume)
{
    double tmp = (double)(volume - PA_VOLUME_MUTED);
    return tmp / (double)(PA_VOLUME_NORM - PA_VOLUME_MUTED);
}

DBusPulseVolume::DBusPulseVolume() :
    QObject()
{
    std::unique_ptr<DBusPropertiesInterface> basicConnectionInterface(new DBusPropertiesInterface("org.PulseAudio1",
                                                            "/org/pulseaudio/server_lookup1",
                                                            QDBusConnection::sessionBus(), 0));

    QDBusReply<QVariant> connection_string = basicConnectionInterface->call(QLatin1String("Get"),
                    QLatin1String("org.PulseAudio.ServerLookup1"),
                    QLatin1String("Address"));

    if (!connection_string.isValid())
    {
        qWarning() << "DBusPulseVolume::DBusPulseVolume(): D-Bus error: " << connection_string.error().message();
    }

    connection_.reset(new QDBusConnection(QDBusConnection::connectToPeer(connection_string.value().toString(), "set-volume")));

    if (connection_->isConnected())
    {
        interface_paths_.reset(new StreamRestoreInterface("org.PulseAudio.Ext.StreamRestore1",
                                                    "/org/pulseaudio/stream_restore1",
                                                    *(connection_.get()), 0));
        if (interface_paths_)
        {
            // get the role paths
            fillRolePath("multimedia");
            fillRolePath("alert");
            fillRolePath("alarm");
            fillRolePath("phone");
        }

        initializeAccountsInterface();
    }
    else
    {
        qWarning() << "DBusPulseVolume::DBusPulseVolume(): Error connecting: " << connection_->lastError().message();
    }
}

DBusPulseVolume::~DBusPulseVolume()
{
}

QString DBusPulseVolume::fillRolePath(QString const &role)
{
    QString role_complete_name = QString("sink-input-by-media-role:") + role;
    // get the role paths
    QDBusReply<QDBusObjectPath> objectPath = interface_paths_->call(QLatin1String("GetEntryByName"), role_complete_name);
    if (!objectPath.isValid())
    {
        qWarning() << "SetVolume::fillRolePath(): D-Bus error: " << objectPath.error().message();
        return "";
    }

    auto role_info = std::make_shared<RoleInformation>();
    role_info->interface_.reset(new DBusPropertiesInterface("org.PulseAudio.Ext.StreamRestore1.RestoreEntry",
                                                objectPath.value().path(),
                                                *(connection_.get()), 0));
    if (!role_info->interface_)
    {
        qWarning() << "SetVolume::fillRolePath() - Error obtaining properties interface";
        return "";
    }
    role_info->path_ = objectPath.value().path();
    roles_map_[role] = role_info;
    return role_info->path_;
}

bool DBusPulseVolume::setVolume(QString const & role, double volume)
{
    if (!interface_paths_)
    {
        qWarning() << "SetVolume::setVolume(): error: Volume interfaces are not initialized";
        return false;
    }

    RolesMap::const_iterator iter = roles_map_.find(role);
    if (iter != roles_map_.end())
    {
        QVariant var;
        PulseaudioVolumeArray t;
        PulseaudioVolume vv(0, volumeDoubleToUint(volume));
        t.addItem(vv);
        var.setValue(t);
        QDBusVariant dbusVar(var);
        QDBusReply<void> set_vol = (*iter).second->interface_->call(QLatin1String("Set"),
                            QVariant::fromValue(QString("org.PulseAudio.Ext.StreamRestore1.RestoreEntry")),
                            QVariant::fromValue(QString("Volume")),
                            QVariant::fromValue(dbusVar));

        if (!set_vol.isValid())
        {
            qWarning() << "SetVolume::setVolume(): D-Bus error: " << set_vol.error().message();
            return false;
        }

        if (accounts_interface_)
        {
            QDBusVariant dbusVar(QVariant::fromValue(volume));
            QDBusReply<void> set_vol = accounts_interface_->call(QLatin1String("Set"),
                                            QVariant::fromValue(QString("com.ubuntu.AccountsService.Sound")),
                                            QVariant::fromValue(QString("Volume")),
                                            QVariant::fromValue(dbusVar));
            if (!set_vol.isValid())
            {
                qWarning() << "SetVolume::setVolume(): D-Bus error: " << set_vol.error().message();
                return false;
            }
        }
    }
    else
    {
        qWarning() << "SetVolume::setVolume(): role " << role << " was not found.";
        return false;
    }
    return true;
}

double DBusPulseVolume::getVolume(QString const & role)
{
    if (interface_paths_)
    {
        RolesMap::const_iterator iter = roles_map_.find(role);
        if (iter != roles_map_.end())
        {
            QDBusReply<QVariant> prev_vol = (*iter).second->interface_->call(QLatin1String("Get"),
                                QLatin1String("org.PulseAudio.Ext.StreamRestore1.RestoreEntry"),
                                QLatin1String("Volume"));

            if (!prev_vol.isValid())
            {
                qWarning() << "SetVolume::setVolume(): D-Bus error: " << prev_vol.error().message();
            }
            QDBusArgument arg = prev_vol.value().value<QDBusArgument>();
            PulseaudioVolumeArray element;
            arg >> element;
            return volumeUIntToDoulbe(element.getItem(0).getVolume());
        }
    }
    return -1.0;
}

void DBusPulseVolume::initializeAccountsInterface()
{
    auto username = qgetenv("USER");
    if (username != "")
    {
        qDebug() << "Setting Accounts interface for user: " << username;
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

bool DBusPulseVolume::waitForVolumeChangedInAccountsService()
{
    if (signal_spy_volume_changed_)
    {
        return signal_spy_volume_changed_->wait();
    }
    else
    {
        qWarning() << "DBusPulseVolume::waitForVolumeChangedInAccountsService(): signal was not instantiated";
    }
    return false;
}
