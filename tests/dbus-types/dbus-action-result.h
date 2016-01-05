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

#include <QtDBus>
#include <QDBusSignature>

class DBusActionResult
{
public:
    DBusActionResult();
    DBusActionResult(bool enabled, QDBusSignature signature, QVariantList value);
    ~DBusActionResult();

    DBusActionResult(const DBusActionResult &other);
    DBusActionResult& operator=(const DBusActionResult &other);

    friend QDBusArgument &operator<<(QDBusArgument &argument, const DBusActionResult &result);
    friend const QDBusArgument &operator>>(const QDBusArgument &argument, DBusActionResult &result);

    bool getEnabled() const;
    QVariantList getValue() const;
    QDBusSignature getSignature() const;

    //register Message with the Qt type system
    static void registerMetaType();

private:
    bool enabled_;
    QDBusSignature signature_;
    QVariantList value_;
};

Q_DECLARE_METATYPE(DBusActionResult)

