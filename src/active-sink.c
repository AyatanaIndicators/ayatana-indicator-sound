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

#include <pulse/error.h>
#include <pulse/gccmacro.h>
#include "active-sink.h"

typedef struct _ActiveSinkPrivate ActiveSinkPrivate;

struct _ActiveSinkPrivate
{
  sink_details* details;
};

#define ACTIVE_SINK_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), ACTIVE_SINK_TYPE, ActiveSinkPrivate))

/* Prototypes */
static void active_sink_class_init (ActiveSinkClass *klass);
static void active_sink_init       (ActiveSink *self);
static void active_sink_dispose    (GObject *object);
static void active_sink_finalize   (GObject *object);

G_DEFINE_TYPE (ActiveSink, active_sink, G_TYPE_OBJECT);

static void
active_sink_class_init (ActiveSinkClass *klass)
{
  GObjectClass      *gobject_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass    *widget_class = GTK_WIDGET_CLASS (klass);

  g_type_class_add_private (klass, sizeof (ActiveSinkPrivate));

  gobject_class->dispose = active_sink_dispose;
  gobject_class->finalize = active_sink_finalize;  
}

static void
active_sink_init(ActiveSink *self)
{
  ActiveSinkPrivate* priv = ACTIVE_SINK_GET_PRIVATE(sink);
  priv->details = NULL;  
}

static void
active_sink_dispose (GObject *object)
{
  ActiveSink * self = ACTIVE_SINK(object);
  ActiveSinkPrivate* priv = ACTIVE_SINK_GET_PRIVATE(sink);

  if (priv->details != NULL) {
    g_free (priv->details->name);
    g_free (priv->details);
  }
  
  G_OBJECT_CLASS (active_sink_parent_class)->dispose (object);
}

static void
active_sink_finalize (GObject *object)
{
  G_OBJECT_CLASS (active_sink_parent_class)->finalize (object);  
}

void
active_sink_update_details (ActiveSink* sink, sink_details* details)
{
  ActiveSinkPrivate* priv = ACTIVE_SINK_GET_PRIVATE(sink);
  priv->details = details;
}

void gboolean
active_sink_is_populated (ActiveSink* sink)
{
  ActiveSinkPrivate* priv = ACTIVE_SINK_GET_PRIVATE(sink);
  return (priv->details != NULL)
}

  