#ifndef __INCLUDE_INDICATOR_SOUND_H__
#define __INCLUDE_INDICATOR_SOUND_H__

/*
A small wrapper utility to load indicators and put them as menu items
into the gnome-panel using it's applet interface.

Copyright 2010 Canonical Ltd.

Authors:
    Conor Curran <conor.curra@canonical.com>
    Ted Gould <ted@canonical.com>

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

#include <libindicator/indicator.h>
#include <libindicator/indicator-object.h>
#include <libindicator/indicator-service-manager.h>

#define INDICATOR_SOUND_TYPE            (indicator_sound_get_type ())
#define INDICATOR_SOUND(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), INDICATOR_SOUND_TYPE, IndicatorSound))
#define INDICATOR_SOUND_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), INDICATOR_SOUND_TYPE, IndicatorSoundClass))
#define IS_INDICATOR_SOUND(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), INDICATOR_SOUND_TYPE))
#define IS_INDICATOR_SOUND_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), INDICATOR_SOUND_TYPE))
#define INDICATOR_SOUND_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), INDICATOR_SOUND_TYPE, IndicatorSoundClass))

typedef struct _IndicatorSound      IndicatorSound;
typedef struct _IndicatorSoundClass IndicatorSoundClass;

//GObject class struct
struct _IndicatorSoundClass {
  IndicatorObjectClass parent_class;
};

//GObject instance struct
struct _IndicatorSound {
  IndicatorObject parent;
  IndicatorServiceManager *service;
};

// GObject Boiler plate
GType indicator_sound_get_type (void);

// Update the accessible description
void update_accessible_desc (IndicatorObject * io);

#endif
