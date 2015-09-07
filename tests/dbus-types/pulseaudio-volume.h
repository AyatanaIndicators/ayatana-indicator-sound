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

class PulseaudioVolume
{
public:
    PulseaudioVolume();
    PulseaudioVolume(unsigned int type, unsigned int volume);
    PulseaudioVolume(const PulseaudioVolume &other);
    PulseaudioVolume& operator=(const PulseaudioVolume &other);
    ~PulseaudioVolume();

    friend QDBusArgument &operator<<(QDBusArgument &argument, PulseaudioVolume const & volume);
    friend const QDBusArgument &operator>>(QDBusArgument const & argument, PulseaudioVolume &volume);

    unsigned int getType() const;
    unsigned int getVolume() const;

    //register Message with the Qt type system
    static void registerMetaType();

private:
    unsigned int type_;
    unsigned int volume_;
};

Q_DECLARE_METATYPE(PulseaudioVolume)


class PulseaudioVolumeArray
{
public:
    PulseaudioVolumeArray();
    PulseaudioVolumeArray(QString const &interface, QString const &property, QDBusVariant const& value);
    PulseaudioVolumeArray(const PulseaudioVolumeArray &other);
    PulseaudioVolumeArray& operator=(const PulseaudioVolumeArray &other);
    ~PulseaudioVolumeArray();

    friend QDBusArgument &operator<<(QDBusArgument &argument, PulseaudioVolumeArray const & volume);
    friend const QDBusArgument &operator>>(QDBusArgument const & argument, PulseaudioVolumeArray &volume);

    int getNumItems() const;
    PulseaudioVolume getItem(int i) const;
    void addItem(PulseaudioVolume const &item);

    //register Message with the Qt type system
    static void registerMetaType();

private:
    QVector<PulseaudioVolume> volume_array_;
};

Q_DECLARE_METATYPE(PulseaudioVolumeArray)
