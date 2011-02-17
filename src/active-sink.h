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

#ifndef __ACTIVE_SINK_H__
#define __ACTIVE_SINK_H__

#include <glib.h>
#include <glib-object.h>

#include "common-defs.h"
#include "sound-service-dbus.h"

#include <pulse/pulseaudio.h>

G_BEGIN_DECLS

#define ACTIVE_SINK_TYPE         (active_sink_get_type ())
#define ACTIVE_SINK(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), ACTIVE_SINK_TYPE, ActiveSink))
#define ACTIVE_SINK_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), ACTIVE_SINK_TYPE, ActiveSinkClass))
#define IS_ACTIVE_SINK(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), ACTIVE_SINK_TYPE))
#define IS_ACTIVE_SINK_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), ACTIVE_SINK_TYPE))
#define ACTIVE_SINK_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), ACTIVE_SINK_TYPE, ActiveSinkClass))

typedef struct _ActiveSink      ActiveSink;
typedef struct _ActiveSinkClass ActiveSinkClass;

struct _ActiveSink {
  GObject parent;
};

struct _ActiveSinkClass {
  GObjectClass parent_class;
};

GType active_sink_get_type  (void) G_GNUC_CONST;

void active_sink_populate (ActiveSink* sink, const pa_sink_info* update);  
void active_sink_update (ActiveSink* sink, const pa_sink_info* update);  
void active_sink_update_voip_input_source (ActiveSink* sink, const pa_source_info* update);
void active_sink_activate_voip_item (ActiveSink* sink);

gboolean active_sink_is_voip_source_populated (ActiveSink* sink);
gboolean active_sink_is_populated (ActiveSink* sink);

void active_sink_determine_blocking_state (ActiveSink* self);

gint active_sink_get_index (ActiveSink* self);
gint active_sink_get_source_index (ActiveSink* self);

SoundState active_sink_get_state (ActiveSink* self);

void active_sink_deactivate (ActiveSink* self);
void active_sink_deactivate_voip_source (ActiveSink* self);
void active_sink_update_mute (ActiveSink* self, gboolean mute_update);
void active_sink_update_volume (ActiveSink* self, gdouble percent);
void active_sink_ensure_sink_is_unmuted (ActiveSink* self);

ActiveSink* active_sink_new (SoundServiceDbus* service);

G_END_DECLS

#endif
