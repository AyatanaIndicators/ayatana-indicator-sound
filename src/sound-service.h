#ifndef __INCLUDE_SOUND_SERVICE_H__
#define __INCLUDE_SOUND_SERVICE_H__

/*
Copyright 2010 Canonical Ltd.

Authors:
    Conor Curran <conor.curran@canonical.com>

This program is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License version 3, as published
by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranties of
MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <config.h>
#include <unistd.h>
#include <glib/gi18n.h>

#include <libindicator/indicator-service.h>

#include "dbus-shared-names.h"

// ENTRY AND EXIT POINTS
void service_shutdown(IndicatorService * service, gpointer user_data);
int main (int argc, char ** argv);

#endif
