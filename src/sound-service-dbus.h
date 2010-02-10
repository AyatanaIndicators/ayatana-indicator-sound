/*
 * Copyright 2010 Canonical Ltd.
 *
 * Authors:
 *     Conor Curran <conor.curran@canonical.com>
 *     Cody Russell <crussell@canonical.com>
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

struct _SoundData
{
  SoundServiceDbus *service;
};

struct _SoundServiceDbus {
  GObject parent;
};

struct _SoundServiceDbusClass {
  GObjectClass parent_class;
  /* Signals -> outward messages to the DBUS and beyond*/
  // TODO - ARE THESE NECESSARY ?
  void (* sink_input_while_muted) (SoundServiceDbus *self, gboolean block_value, gpointer sound_data);
  void (* sink_volume_update) (SoundServiceDbus *self, gdouble sink_volume, gpointer sound_data);
}; 
GType sound_service_dbus_get_type  (void) G_GNUC_CONST;

// Utility methods to get the messages across into the sound-service-dbus
void sound_service_dbus_sink_input_while_muted (SoundServiceDbus* obj, gboolean block_value);
void sound_service_dbus_update_sink_volume(SoundServiceDbus* obj, gdouble sink_volume);
void sound_service_dbus_update_sink_mute(SoundServiceDbus* obj, gboolean sink_mute);

// DBUS METHODS
void sound_service_dbus_set_sink_volume(SoundServiceDbus* service, const guint volume_percent, GError** gerror);

G_END_DECLS

#endif
