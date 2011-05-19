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

#ifndef _SOUND_STATE_H_
#define _SOUND_STATE_H_

#include <glib.h>
#include "common-defs.h"

/* Helper functions for determining SOUNDSTATE */

SoundState sound_state_get_from_volume (int volume_percent);

#endif /* _SOUND_STATE_H_ */

