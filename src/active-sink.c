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

#include <libdbusmenu-glib/menuitem.h>

#include "active-sink.h"
#include "slider-menu-item.h"
#include "mute-menu-item.h"



typedef struct _ActiveSinkPrivate ActiveSinkPrivate;

struct _ActiveSinkPrivate
{
  gboolean            is_active;
  SliderMenuItem*     volume_slider_menuitem;
  MuteMenuItem*       mute_menuitem;
  SoundState          current_sound_state;  
  gint                index;
  SoundServiceDbus*   service;
};

#define ACTIVE_SINK_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), ACTIVE_SINK_TYPE, ActiveSinkPrivate))

/* Prototypes */
static void active_sink_class_init (ActiveSinkClass *klass);
static void active_sink_init       (ActiveSink *self);
static void active_sink_dispose    (GObject *object);
static void active_sink_finalize   (GObject *object);

static SoundState active_sink_get_state_from_volume (ActiveSink* self);

G_DEFINE_TYPE (ActiveSink, active_sink, G_TYPE_OBJECT);

static void
active_sink_class_init (ActiveSinkClass *klass)
{
  GObjectClass      *gobject_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (ActiveSinkPrivate));

  gobject_class->dispose = active_sink_dispose;
  gobject_class->finalize = active_sink_finalize;  
}

static void
active_sink_init(ActiveSink *self)
{
  ActiveSinkPrivate* priv = ACTIVE_SINK_GET_PRIVATE (self);
  priv->details = NULL;  
  priv->mute_menuitem = NULL;
  priv->volume_slider_menuitem = NULL;
  priv->current_sound_state = UNAVAILABLE;
  priv->is_active = TRUE;  
  priv->index = -1;

  // Init our menu items.
  priv->mute_menuitem = g_object_new (MUTE_MENU_ITEM_TYPE, NULL);
  priv->volume_slider_menuitem = g_object_new (SLIDER_MENU_ITEM_TYPE, NULL);  
}

static void
active_sink_dispose (GObject *object)
{
  ActiveSink * self = ACTIVE_SINK(object);
  ActiveSinkPrivate* priv = ACTIVE_SINK_GET_PRIVATE (self);

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

// This needs to populate the appropriate values on the menu items
void
active_sink_populate (ActiveSink* sink,
                      gdouble volume,
                      gboolean mute,
                      gint device_index)
{
  ActiveSinkPrivate* priv = ACTIVE_SINK_GET_PRIVATE(sink);
  active_sink_update_volume (sink, volume);
  active_sink_update_mute (sink, mute);
  priv->index = device_index;
  priv->is_active = TRUE;
}

gboolean
active_sink_is_populated (ActiveSink* sink)
{
  ActiveSinkPrivate* priv = ACTIVE_SINK_GET_PRIVATE (sink);
  return priv->is_active;
}

gboolean
active_sink_is_muted (ActiveSink* self)
{
  ActiveSinkPrivate* priv = ACTIVE_SINK_GET_PRIVATE (self);
  return mute_menu_item_is_muted (priv->mute_menuitem);
}

gint
active_sink_get_index (ActiveSink* self)
{
  ActiveSinkPrivate* priv = ACTIVE_SINK_GET_PRIVATE (self);
  return priv->index;
}

void 
active_sink_update_volume (ActiveSink* self, gdouble percent)
{
  ActiveSinkPrivate* priv = ACTIVE_SINK_GET_PRIVATE (self);
  slider_menu_item_update (priv->volume_slider_menuitem, percent);  
  sound_service_dbus_update_sound_state (priv->service,
                                         active_sink_get_state_from_volume (self));
}

void 
active_sink_update_mute (ActiveSink* self, gboolean muted)
{
  ActiveSinkPrivate* priv = ACTIVE_SINK_GET_PRIVATE (self);
  mute_menu_item_update (priv->mute_menuitem, muted);
  SoundState state = active_sink_get_state_from_volume (self);
  
  if (muted == TRUE){
    state = MUTED;
  }
  sound_service_dbus_update_sound_state (priv->service, state);
}

SoundState
active_sink_get_state (ActiveSink* self)
{
  ActiveSinkPrivate* priv = ACTIVE_SINK_GET_PRIVATE (self);
  return priv->current_sound_state;
}


static SoundState
active_sink_get_state_from_volume (ActiveSink* self)
{
  ActiveSinkPrivate* priv = ACTIVE_SINK_GET_PRIVATE (self);
  GVariant* v = dbusmenu_menuitem_property_get_variant (DBUSMENU_MENUITEM(priv->volume_slider_menuitem),
                                                        DBUSMENU_VOLUME_MENUITEM_LEVEL);
  gdouble volume_percent = g_variant_get_double (v);

  SoundState state = LOW_LEVEL;

  if (volume_percent < 30.0 && volume_percent > 0) {
    state = LOW_LEVEL;
  } 
  else if (volume_percent < 70.0 && volume_percent >= 30.0) {
    state = MEDIUM_LEVEL;
  } 
  else if (volume_percent >= 70.0) {
    state = HIGH_LEVEL;
  } 
  else if (volume_percent == 0.0) {
    state = ZERO_LEVEL;
  }
  return state;
}

ActiveSink* active_sink_new (SoundServiceDbus* service)
{
  ActiveSink* sink = g_object_new (ACTIVE_SINK_TYPE, NULL);
  ActiveSinkPrivate* priv = ACTIVE_SINK_GET_PRIVATE (sink);
  priv->service = service;
  sound_service_dbus_build_sound_menu (service,
                                       priv->mute_menuitem,
                                       priv->voluem_slider_menuitem);
  return sink;
}
