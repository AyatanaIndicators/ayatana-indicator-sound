#ifndef __INCLUDE_PULSE_MANAGER_H__
#define __INCLUDE_PULSE_MANAGER_H__
/*
A small wrapper utility to load indicators and put them as menu items
into the gnome-panel using it's applet interface.

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


#include <pulse/pulseaudio.h>
#include <glib.h>
#include "sound-service-dbus.h"


typedef struct {
    gchar* name;
    gchar* description;
    gchar* icon_name;
    gint index;
    gint device_index;
    pa_cvolume volume;
    pa_channel_map channel_map;
    gboolean mute;
    gboolean active_port;
    pa_volume_t base_volume;
} sink_info;


pa_context* get_context(void);
void establish_pulse_activities(SoundServiceDbus *service);
void set_sink_volume(gdouble percent);
void toggle_global_mute(gboolean mute_value); 
void close_pulse_activites();

#endif

