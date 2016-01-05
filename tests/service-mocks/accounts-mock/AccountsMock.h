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

#include <QDBusContext>
#include <QDBusObjectPath>
#include <QObject>

namespace ubuntu
{

namespace indicators
{

namespace testing
{

class AccountsMock : public QObject, protected QDBusContext
{
    Q_OBJECT

public Q_SLOTS:
    QDBusObjectPath FindUserByName(QString const & username) const;
    QDBusObjectPath FindUserById(int64_t uid) const;

public:
    AccountsMock(QObject* parent = 0);
    virtual ~AccountsMock();
};

} // namespace testing

} // namespace indicators

} // namespace ubuntu
