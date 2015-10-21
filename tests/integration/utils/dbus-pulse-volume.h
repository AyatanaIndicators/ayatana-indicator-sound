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

#include <QObject>
#include <QDBusConnection>

#include <map>
#include <memory>

class StreamRestoreInterface;
class DBusPropertiesInterface;
class AccountsInterface;
class AccountsSoundInterface;
class QSignalSpy;

struct RoleInformation
{
    std::unique_ptr<DBusPropertiesInterface> interface_;
    QString path_;
};

class DBusPulseVolume : public QObject
{
public:
    DBusPulseVolume();
    ~DBusPulseVolume();

    bool setVolume(QString const & role, double volume);
    double getVolume(QString const & role);
    bool waitForVolumeChangedInAccountsService();

protected:
    QString fillRolePath(QString const &role);
    void initializeAccountsInterface();
    std::unique_ptr<QDBusConnection> connection_;
    std::unique_ptr<StreamRestoreInterface> interface_paths_;
    typedef std::map<QString, std::shared_ptr<RoleInformation>> RolesMap;
    RolesMap roles_map_;
    std::unique_ptr<DBusPropertiesInterface> accounts_interface_;
    std::unique_ptr<QSignalSpy> signal_spy_volume_changed_;
};
