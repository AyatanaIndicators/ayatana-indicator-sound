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
    notifyPropertyChanged(ACCOUNTS_SOUND_INTERFACE,
                          USER_PATH,
                          "Volume");
}

void AccountsServiceSoundMock::notifyPropertyChanged(QString const & interface,
                                                     QString const & path,
                                                     QString const & propertyName)
{
    QDBusMessage signal = QDBusMessage::createSignal(
        path,
        "org.freedesktop.DBus.Properties",
        "PropertiesChanged");
    signal << interface;
    QVariantMap changedProps;
    changedProps.insert(propertyName, property(propertyName.toStdString().c_str()));
    signal << changedProps;
    signal << QStringList();
    QDBusConnection::systemBus().send(signal);
}
