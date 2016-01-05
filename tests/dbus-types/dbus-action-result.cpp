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
#include "dbus-action-result.h"

DBusActionResult::DBusActionResult()
    : enabled_(false)
{}

DBusActionResult::~DBusActionResult()
{}

DBusActionResult::DBusActionResult(bool enabled, QDBusSignature signature, QVariantList value)
    : enabled_(enabled)
    , signature_(signature)
    , value_(value)
{
}

DBusActionResult::DBusActionResult(const DBusActionResult &other)
    : enabled_(other.enabled_)
    , signature_(other.signature_)
    , value_(other.value_)
{
}

DBusActionResult& DBusActionResult::operator=(const DBusActionResult &other)
{
    enabled_ = other.enabled_;
    signature_ = other.signature_;
    value_ = other.value_;

    return *this;
}

QVariantList DBusActionResult::getValue() const
{
    return value_;
}

bool DBusActionResult::getEnabled() const
{
    return enabled_;
}

QDBusSignature DBusActionResult::getSignature() const
{
    return signature_;
}

//register Message with the Qt type system
void DBusActionResult::registerMetaType()
{
    qRegisterMetaType<DBusActionResult>("DBusActionResult");

    qDBusRegisterMetaType<DBusActionResult>();
}

QDBusArgument &operator<<(QDBusArgument &argument, const DBusActionResult &result)
{
    argument.beginStructure();
    argument << result.enabled_;
    argument << result.signature_;
    argument << result.value_;
    argument.endStructure();

    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, DBusActionResult &result)
{
    argument.beginStructure();
    argument >> result.enabled_;
    argument >> result.signature_;
    argument >> result.value_;
    argument.endStructure();

    return argument;
}
