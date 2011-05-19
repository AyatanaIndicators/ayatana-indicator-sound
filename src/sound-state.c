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

#include "config.h"

#include "sound-state.h"

SoundState
sound_state_get_from_volume (int volume_percent)
{
  SoundState state = LOW_LEVEL;
  
  if (volume_percent < 30 && volume_percent > 0) {
    state = LOW_LEVEL;
  } 
  else if (volume_percent < 70 && volume_percent >= 30) {
    state = MEDIUM_LEVEL;
  } 
  else if (volume_percent >= 70) {
    state = HIGH_LEVEL;
  } 
  else if (volume_percent <= 0) {
    state = ZERO_LEVEL;
  }
  return state;
}

