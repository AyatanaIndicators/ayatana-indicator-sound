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

#include "AccountsMock.h"
#include "AccountsDefs.h"

using namespace ubuntu::indicators::testing;

AccountsMock::AccountsMock(QObject* parent)
    : QObject(parent)
{
}

AccountsMock::~AccountsMock() = default;

QDBusObjectPath AccountsMock::FindUserByName(QString const & username) const
{
    return QDBusObjectPath(USER_PATH);
}

QDBusObjectPath AccountsMock::FindUserById(int64_t uid) const
{
    return QDBusObjectPath(USER_PATH);
}
