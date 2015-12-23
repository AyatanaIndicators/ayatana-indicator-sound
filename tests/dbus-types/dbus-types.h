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

#include <QDBusMetaType>
#include "pulseaudio-volume.h"

namespace DBusTypes
{
    inline void registerMetaTypes()
    {
        PulseaudioVolume::registerMetaType();
        PulseaudioVolumeArray::registerMetaType();
    }

    static constexpr char const* DBUS_NAME = "com.canonical.indicator.sound";

    static constexpr char const* DBUS_PULSE = "org.PulseAudio1";

    static constexpr char const* ACCOUNTS_SERVICE = "org.freedesktop.Accounts";

    static constexpr char const* STREAM_RESTORE_NAME = "org.PulseAudio.Ext.StreamRestore1";

    static constexpr char const* STREAM_RESTORE_PATH = "/org/pulseaudio/stream_restore1";

    static constexpr char const* STREAM_RESTORE_ENTRY_NAME = "org.PulseAudio.Ext.StreamRestore1.RestoreEntry";

    static constexpr char const* MAIN_SERVICE_PATH = "/com/canonical/indicator/sound";

    static constexpr char const* ACTIONS_INTERFACE = "org.gtk.Actions";

} // namespace DBusTypes

