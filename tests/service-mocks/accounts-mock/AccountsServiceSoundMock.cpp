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
#include <QDebug>
#include <QDBusMessage>
#include <QDBusConnection>

#include "AccountsServiceSoundMock.h"
#include "AccountsDefs.h"

using namespace ubuntu::indicators::testing;

AccountsServiceSoundMock::AccountsServiceSoundMock(QObject* parent)
    : QObject(parent)
    , volume_(0.0)
{
}

AccountsServiceSoundMock::~AccountsServiceSoundMock() = default;

double AccountsServiceSoundMock::volume() const
{
    return volume_;
}

void AccountsServiceSoundMock::setVolume(double volume)
{
    volume_ = volume;
    notifier_.notifyPropertyChanged(QDBusConnection::systemBus(),
                                    ACCOUNTS_SOUND_INTERFACE,
                                    USER_PATH,
                                    "Volume",
                                    property("Volume"));
}

QString AccountsServiceSoundMock::lastRunningPlayer() const
{
    return lastRunningPlayer_;
}

void AccountsServiceSoundMock::setLastRunningPlayer(QString const & lastRunningPlayer)
{
    lastRunningPlayer_ = lastRunningPlayer;
    notifier_.notifyPropertyChanged(QDBusConnection::systemBus(),
                                    ACCOUNTS_SOUND_INTERFACE,
                                    USER_PATH,
                                    "LastRunningPlayer",
                                    property("LastRunningPlayer"));
}
