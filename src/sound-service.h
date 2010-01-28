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

#include <pulse/pulseaudio.h>
#include <pulse/glib-mainloop.h>
#include <pulse/error.h>
#include <pulse/gccmacro.h>

#include "dbus-shared-names.h"

// GTK + DBUS
static GMainLoop * mainloop = NULL;
static DbusmenuMenuitem * root_menuitem = NULL;
static DbusmenuMenuitem * mute_all_menuitem = NULL;
static SoundServiceDbus * dbus_interface = NULL;

// PULSEAUDIO
static pa_context *pulse_context = NULL;
static pa_glib_mainloop *pa_main_loop = NULL;
static GPtrArray* sink_list = NULL;
static gboolean sink_available = TRUE;

static void context_state_callback(pa_context *c, void *userdata);
static gboolean idle_routine (gpointer data);
static void rebuild_sound_menu(DbusmenuMenuitem *root, SoundServiceDbus *service);

static gboolean all_muted = FALSE;
static void set_global_mute();
//static void set_volume(gint sink_index, gint volume_percent);

typedef struct {
    gchar* name;
    gchar* description;
    gchar* icon_name;
    gint index;
    gint device_index;
    pa_cvolume volume;
    pa_channel_map channel_map;
    gboolean mute;
} device_info;

// ENTRY AND EXIT POINTS
void service_shutdown(IndicatorService * service, gpointer user_data);
int main (int argc, char ** argv);

#endif

