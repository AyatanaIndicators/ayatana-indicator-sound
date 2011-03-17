/*
Copyright 2011 Canonical Ltd.

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

#include "device.h"

void pm_establish_pulse_connection (Device* device);
void close_pulse_activites();
void pm_update_volume (gint sink_index, pa_cvolume new_volume);
void pm_update_mic_gain (gint source_index, pa_cvolume new_gain);
void pm_update_mic_mute (gint source_index, int mute_update);
void pm_update_mute (gboolean update);






