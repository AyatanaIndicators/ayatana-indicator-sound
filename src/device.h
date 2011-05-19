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

#ifndef __DEVICE_H__
#define __DEVICE_H__

#include <glib.h>
#include <glib-object.h>

#include "common-defs.h"
#include "sound-service-dbus.h"

#include <pulse/pulseaudio.h>

G_BEGIN_DECLS

#define DEVICE_TYPE         (device_get_type ())
#define DEVICE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), DEVICE_TYPE, Device))
#define DEVICE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), DEVICE_TYPE, DeviceClass))
#define IS_DEVICE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), DEVICE_TYPE))
#define IS_DEVICE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), DEVICE_TYPE))
#define DEVICE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), DEVICE_TYPE, DeviceClass))

typedef struct _Device      Device;
typedef struct _DeviceClass DeviceClass;

struct _Device {
  GObject parent;
};

struct _DeviceClass {
  GObjectClass parent_class;
};

GType device_get_type  (void) G_GNUC_CONST;

/**
 * TODO
 * Refactor this to become a device manager obj basically acting as wrapper for
 * the communication between pulseaudio-mgr and the individual items.
 * First steps collapse slider/volume related stuff into slider-menu-item.
 */

// Sink related
void device_sink_populate (Device* sink, const pa_sink_info* update);
void device_sink_update (Device* sink, const pa_sink_info* update);
gboolean device_is_sink_populated (Device* sink);
gint device_get_sink_index (Device* self);
void device_sink_deactivated (Device* self);
void device_update_mute (Device* self, gboolean mute_update);
void device_ensure_sink_is_unmuted (Device* self);

// source and sinkinput/client related for VOIP functionality
void device_update_voip_input_source (Device* sink, const pa_source_info* update);
void device_activate_voip_item (Device* sink, gint source_output_index, gint client_index);
gint device_get_voip_source_output_index (Device* sink);
gboolean device_is_voip_source_populated (Device* sink);
gint device_get_source_index (Device* self);
void device_determine_blocking_state (Device* self);
void device_deactivate_voip_source (Device* self, gboolean visible);
void device_deactivate_voip_client (Device* self);
SoundState device_get_state (Device* self);

Device* device_new (SoundServiceDbus* service);

G_END_DECLS

#endif
