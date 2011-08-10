/*
 * Copyright 2010 Canonical Ltd.
 *
 * Authors:
 *     Conor Curran <conor.curran@canonical.com>
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
 */

#ifndef __SOUND_SERVICE_DBUS_H__
#define __SOUND_SERVICE_DBUS_H__

#include <glib.h>
#include <glib-object.h>
#include <libdbusmenu-glib/menuitem.h>
#include "common-defs.h"


G_BEGIN_DECLS

#define SOUND_SERVICE_DBUS_TYPE         (sound_service_dbus_get_type ())
#define SOUND_SERVICE_DBUS(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), SOUND_SERVICE_DBUS_TYPE, SoundServiceDbus))
#define SOUND_SERVICE_DBUS_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), SOUND_SERVICE_DBUS_TYPE, SoundServiceDbusClass))
#define IS_SOUND_SERVICE_DBUS(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), SOUND_SERVICE_DBUS_TYPE))
#define IS_SOUND_SERVICE_DBUS_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), SOUND_SERVICE_DBUS_TYPE))
#define SOUND_SERVICE_DBUS_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), SOUND_SERVICE_DBUS_TYPE, SoundServiceDbusClass))

typedef struct _SoundServiceDbus      SoundServiceDbus;
typedef struct _SoundServiceDbusClass SoundServiceDbusClass;
typedef struct _SoundData             SoundData;

struct _SoundData {
  SoundServiceDbus *service;
};

struct _SoundServiceDbus {
  GObject parent;
};

struct _SoundServiceDbusClass {
  GObjectClass parent_class;
};

GType sound_service_dbus_get_type  (void) G_GNUC_CONST;

DbusmenuMenuitem* sound_service_dbus_create_root_item (SoundServiceDbus* self, gboolean greeter_mode);
void sound_service_dbus_update_sound_state (SoundServiceDbus* self, SoundState new_state);
void sound_service_dbus_build_sound_menu ( SoundServiceDbus* self,
                                           DbusmenuMenuitem* mute_item,
                                           DbusmenuMenuitem* slider_item,
                                           DbusmenuMenuitem* voip_input_menu_item);


G_END_DECLS

#endif
