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
#include "dbus-types.h"

PulseaudioVolume::PulseaudioVolume() :
        type_(0),
        volume_(10)
{
}

PulseaudioVolume::PulseaudioVolume(unsigned int type, unsigned int volume) :
          type_(type)
        , volume_(volume)
{
}

PulseaudioVolume::PulseaudioVolume(const PulseaudioVolume &other) :
        type_(other.type_),
        volume_(other.volume_)
{
}

PulseaudioVolume& PulseaudioVolume::operator=(const PulseaudioVolume &other)
{
    type_ = other.type_;
    volume_ = other.volume_;

    return *this;
}

PulseaudioVolume::~PulseaudioVolume()
{
}

unsigned int PulseaudioVolume::getType() const
{
    return type_;
}

unsigned int PulseaudioVolume::getVolume() const
{
    return volume_;
}

void PulseaudioVolume::registerMetaType()
{
    qRegisterMetaType<PulseaudioVolume>("PulseaudioVolume");

    qDBusRegisterMetaType<PulseaudioVolume>();
}

QDBusArgument &operator<<(QDBusArgument &argument, const PulseaudioVolume& volume)
{
    argument.beginStructure();
    argument << volume.type_;
    argument << volume.volume_;
    argument.endStructure();

    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, PulseaudioVolume &volume)
{
    argument.beginStructure();
    argument >> volume.type_;
    argument >> volume.volume_;
    argument.endStructure();

    return argument;
}

PulseaudioVolumeArray::PulseaudioVolumeArray()
{
}

PulseaudioVolumeArray::PulseaudioVolumeArray(const PulseaudioVolumeArray &other) :
          volume_array_(other.volume_array_)
{
}

PulseaudioVolumeArray& PulseaudioVolumeArray::operator=(const PulseaudioVolumeArray &other)
{
    volume_array_ = other.volume_array_;

    return *this;
}

PulseaudioVolumeArray::~PulseaudioVolumeArray()
{
}

int PulseaudioVolumeArray::getNumItems() const
{
    return volume_array_.size();
}

PulseaudioVolume PulseaudioVolumeArray::getItem(int i) const
{
    if (i < volume_array_.size())
    {
        return volume_array_[i];
    }
    return PulseaudioVolume();
}

void PulseaudioVolumeArray::addItem(PulseaudioVolume const &item)
{
    volume_array_.push_back(item);
}

void PulseaudioVolumeArray::registerMetaType()
{
    qRegisterMetaType<PulseaudioVolumeArray>("PulseaudioVolumeArray");

    qDBusRegisterMetaType<PulseaudioVolumeArray>();
}

QDBusArgument &operator<<(QDBusArgument &argument, const PulseaudioVolumeArray& volume)
{
    argument.beginArray( qMetaTypeId<PulseaudioVolume>() );
    for (int i = 0; i < volume.volume_array_.size(); ++ i)
    {
        PulseaudioVolume item = volume.getItem(i);
        argument << item;
    }
    argument.endArray();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, PulseaudioVolumeArray &volume)
{
    argument.beginArray();
    while ( !argument.atEnd() ) {
        PulseaudioVolume item;
        argument >> item;
        volume.volume_array_.push_back(item);
    }
    argument.endArray();

    return argument;
}
