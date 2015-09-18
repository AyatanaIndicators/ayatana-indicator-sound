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
#include <QObject>

class AccountsServiceSoundMock : public QObject, protected QDBusContext
{
    Q_OBJECT
//    Q_CLASSINFO("D-Bus Interface", "test.com.ubuntu.AccountsService.Sound")
    Q_PROPERTY(double Volume READ volume WRITE setVolume)

public Q_SLOTS:
    double volume() const;
    void setVolume(double volume);

public:
    AccountsServiceSoundMock(QObject* parent = 0);
    virtual ~AccountsServiceSoundMock();

protected:
    void notifyPropertyChanged(QString const & interface,
                                QString const & path,
                                QString const & propertyName);

private:
    double volume_;
};
