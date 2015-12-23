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

#include "DBusPropertiesNotifier.h"

namespace ubuntu
{

namespace indicators
{

namespace testing
{

class MediaPlayerMprisMock : public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_PROPERTY(bool CanPlay READ canPlay WRITE setCanPlay)
    Q_PROPERTY(bool CanPause READ canPause WRITE setCanPause)
    Q_PROPERTY(bool CanGoNext READ canGoNext WRITE setCanGoNext)
    Q_PROPERTY(bool CanGoPrevious READ canGoPrevious WRITE setCanGoPrevious)
    Q_PROPERTY(QString DesktopEntry READ desktopEntry WRITE setDesktopEntry)

public Q_SLOTS:
    bool canPlay() const;
    void setCanPlay(bool canPlay);

    bool canPause() const;
    void setCanPause(bool canPause);

    bool canGoNext() const;
    void setCanGoNext(bool canGoNext);

    bool canGoPrevious() const;
    void setCanGoPrevious(bool canGoPrevious);

    QString desktopEntry() const;
    void setDesktopEntry(QString const &destopEntry);

public:
    MediaPlayerMprisMock(QString const &playerName, QObject* parent = 0);
    virtual ~MediaPlayerMprisMock();

private:
    bool can_play_;
    bool can_pause_;
    bool can_gonext_;
    bool can_goprevious_;
    DBusPropertiesNotifier notifier_;
    QString player_name_;
};

} // namespace testing

} // namespace indicators

} // namespace ubuntu
