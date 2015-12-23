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

#include "MediaPlayerMprisMock.h"

using namespace ubuntu::indicators::testing;

MediaPlayerMprisMock::MediaPlayerMprisMock(QString const &playerName, QObject* parent)
    : QObject(parent)
    , can_play_(true)
    , can_pause_(true)
    , can_gonext_(true)
    , can_goprevious_(true)
    , player_name_(playerName)
{
}

MediaPlayerMprisMock::~MediaPlayerMprisMock() = default;

bool MediaPlayerMprisMock::canPlay() const
{
    return can_play_;
}

void MediaPlayerMprisMock::setCanPlay(bool canPlay)
{
    can_play_ = canPlay;
    notifier_.notifyPropertyChanged(QDBusConnection::sessionBus(),
                                    "org.mpris.MediaPlayer2.Player",
                                    "/org/mpris/MediaPlayer2",
                                    "CanPlay",
                                    property("CanPlay"));
}

bool MediaPlayerMprisMock::canPause() const
{
    return can_pause_;
}

void MediaPlayerMprisMock::setCanPause(bool canPause)
{
    can_pause_ = canPause;
    notifier_.notifyPropertyChanged(QDBusConnection::sessionBus(),
                                    "org.mpris.MediaPlayer2.Player",
                                    "/org/mpris/MediaPlayer2",
                                    "CanPause",
                                    property("CanPause"));
}

bool MediaPlayerMprisMock::canGoNext() const
{
    return can_gonext_;
}

void MediaPlayerMprisMock::setCanGoNext(bool canGoNext)
{
    can_gonext_ = canGoNext;
    notifier_.notifyPropertyChanged(QDBusConnection::sessionBus(),
                                    "org.mpris.MediaPlayer2.Player",
                                    "/org/mpris/MediaPlayer2",
                                    "CanGoNext",
                                    property("CanGoNext"));
}

bool MediaPlayerMprisMock::canGoPrevious() const
{
    return can_goprevious_;
}

void MediaPlayerMprisMock::setCanGoPrevious(bool canGoPrevious)
{
    can_goprevious_ = canGoPrevious;
    notifier_.notifyPropertyChanged(QDBusConnection::sessionBus(),
                                    "org.mpris.MediaPlayer2.Player",
                                    "/org/mpris/MediaPlayer2",
                                    "CanGoPrevious",
                                    property("CanGoPrevious"));
}

QString MediaPlayerMprisMock::desktopEntry() const
{
    return player_name_;
}

void MediaPlayerMprisMock::setDesktopEntry(QString const &)
{
}
