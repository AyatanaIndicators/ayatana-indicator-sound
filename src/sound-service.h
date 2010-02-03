#ifndef __INCLUDE_SOUND_SERVICE_H__
#define __INCLUDE_SOUND_SERVICE_H__

/*
This service primarily controls PulseAudio and is driven by the sound indicator menu on the panel.
Copyright 2010 Canonical Ltd.

Authors:
    Conor Curran <conor.curran@canonical.com>
    Ted Gould <ted@canonical.com>
    Christoph Korn <c_korn@gmx.de>
    Cody Russell <crussell@canonical.com>

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

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

#include <libdbusmenu-glib/server.h>
#include <libdbusmenu-glib/menuitem.h>
#include <libdbusmenu-glib/client.h>

#include <libindicator/indicator-service.h>

#include "dbus-shared-names.h"


// ENTRY AND EXIT POINTS
void service_shutdown(IndicatorService * service, gpointer user_data);
int main (int argc, char ** argv);
void update_pa_state(gboolean pa_state, gboolean sink_available, gboolean sink_muted, gdouble current_vol);

#endif

