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

#include <string>
#include "dbus-pulse-volume.h"

int main(int argc, char **argv)
{
    DBusTypes::registerMetaTypes();
    if (argc == 2)
    {
        DBusPulseVolume volume;
        volume.getVolume(argv[1]);
    }
    return 0;
}
