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

#include "DBusPropertiesNotifier.h"

#include <QDBusMessage>

using namespace ubuntu::indicators::testing;

void DBusPropertiesNotifier::notifyPropertyChanged(QDBusConnection const & connection,
                                                   QString const & interface,
                                                   QString const & path,
                                                   QString const & propertyName,
                                                   QVariant const & propertyValue)
{
    QDBusMessage signal = QDBusMessage::createSignal(
        path,
        "org.freedesktop.DBus.Properties",
        "PropertiesChanged");
    signal << interface;
    QVariantMap changedProps;
    changedProps.insert(propertyName, propertyValue);
    signal << changedProps;
    signal << QStringList();
    connection.send(signal);
}
