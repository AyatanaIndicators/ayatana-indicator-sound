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
#include "voip-input-menu-item.h"
#include "pulseaudio-mgr.h"

typedef struct _ActiveSinkPrivate ActiveSinkPrivate;

struct _ActiveSinkPrivate
{
  SliderMenuItem*     volume_slider_menuitem;
  MuteMenuItem*       mute_menuitem;
  VoipInputMenuItem*  voip_input_menu_item;
  SoundState          current_sound_state;
  SoundServiceDbus*   service;
};

#define ACTIVE_SINK_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), ACTIVE_SINK_TYPE, ActiveSinkPrivate))

/* Prototypes */
static void active_sink_class_init (ActiveSinkClass *klass);
static void active_sink_init       (ActiveSink *self);
static void active_sink_dispose    (GObject *object);
static void active_sink_finalize   (GObject *object);

static SoundState active_sink_get_state_from_volume (ActiveSink* self);
static void active_sink_volume_update (ActiveSink* self, gdouble percent);
static void active_sink_mute_update (ActiveSink* self, gboolean muted);

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
active_sink_init (ActiveSink *self)
{
  ActiveSinkPrivate* priv = ACTIVE_SINK_GET_PRIVATE (self);
  priv->mute_menuitem = NULL;
  priv->volume_slider_menuitem = NULL;
  priv->voip_input_menu_item = NULL;
  priv->current_sound_state = UNAVAILABLE;
  priv->service = NULL;

  // Init our menu items.
  priv->mute_menuitem = g_object_new (MUTE_MENU_ITEM_TYPE, NULL);
  priv->voip_input_menu_item = g_object_new (VOIP_INPUT_MENU_ITEM_TYPE, NULL);;
  priv->volume_slider_menuitem = slider_menu_item_new (self);
  mute_menu_item_enable (priv->mute_menuitem, FALSE);
  slider_menu_item_enable (priv->volume_slider_menuitem, FALSE);  
}

static void
active_sink_dispose (GObject *object)
{  
  G_OBJECT_CLASS (active_sink_parent_class)->dispose (object);
}

static void
active_sink_finalize (GObject *object)
{
  G_OBJECT_CLASS (active_sink_parent_class)->finalize (object);  
}

void
active_sink_populate (ActiveSink* sink,
                      const pa_sink_info* update)
{
  ActiveSinkPrivate* priv = ACTIVE_SINK_GET_PRIVATE(sink);
  active_sink_mute_update (sink, update->mute);
  mute_menu_item_enable (priv->mute_menuitem, TRUE);
  slider_menu_item_populate (priv->volume_slider_menuitem, update);
  SoundState state = active_sink_get_state_from_volume (sink);
  if (priv->current_sound_state != state){
    priv->current_sound_state  = state;
    sound_service_dbus_update_sound_state (priv->service,
                                           priv->current_sound_state);
  }

}

void
active_sink_activate_voip_item (ActiveSink* self, gint sink_input_index, gint client_index)
{
  ActiveSinkPrivate* priv = ACTIVE_SINK_GET_PRIVATE (self);
  if (voip_input_menu_item_is_interested (priv->voip_input_menu_item,
                                          sink_input_index,
                                          client_index)){
    voip_input_menu_item_enable (priv->voip_input_menu_item, TRUE);
  }
}

void
active_sink_deactivate_voip_source (ActiveSink* self, gboolean visible)
{
  ActiveSinkPrivate* priv = ACTIVE_SINK_GET_PRIVATE (self);
  visible &= voip_input_menu_item_is_active (priv->voip_input_menu_item);
  voip_input_menu_item_deactivate_source (priv->voip_input_menu_item, visible);
}

void
active_sink_deactivate_voip_client (ActiveSink* self)
{
  ActiveSinkPrivate* priv = ACTIVE_SINK_GET_PRIVATE (self);
  voip_input_menu_item_deactivate_voip_client (priv->voip_input_menu_item);
}

void
active_sink_update (ActiveSink* sink,
                    const pa_sink_info* update)
{
  ActiveSinkPrivate* priv = ACTIVE_SINK_GET_PRIVATE (sink);
  slider_menu_item_update (priv->volume_slider_menuitem, update);

  SoundState state = active_sink_get_state_from_volume (sink);
  if (priv->current_sound_state != state){
    priv->current_sound_state  = state;
    sound_service_dbus_update_sound_state (priv->service,
                                           priv->current_sound_state);
  }

  active_sink_mute_update (sink, update->mute);  
}

gint
active_sink_get_current_sink_input_index (ActiveSink* sink)
{
  ActiveSinkPrivate* priv = ACTIVE_SINK_GET_PRIVATE (sink);
  return voip_input_menu_item_get_sink_input_index (priv->voip_input_menu_item);
}

static void 
active_sink_mute_update (ActiveSink* self, gboolean muted)
{
  ActiveSinkPrivate* priv = ACTIVE_SINK_GET_PRIVATE (self);
  mute_menu_item_update (priv->mute_menuitem, muted);
  SoundState state = active_sink_get_state_from_volume (self);
  
  if (muted == TRUE){
    state = MUTED;
  }
  // Only send signals if something has changed
  if (priv->current_sound_state != state){
    priv->current_sound_state = state;
    sound_service_dbus_update_sound_state (priv->service, state);
  }
}

void
active_sink_ensure_sink_is_unmuted (ActiveSink* self)
{
  ActiveSinkPrivate* priv = ACTIVE_SINK_GET_PRIVATE (self);
  if (mute_menu_item_is_muted (priv->mute_menuitem)){
    pm_update_mute (FALSE);    
  }
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

void
active_sink_determine_blocking_state (ActiveSink* self)
{
  ActiveSinkPrivate* priv = ACTIVE_SINK_GET_PRIVATE (self);
  if (mute_menu_item_is_muted (priv->mute_menuitem)){
    /**
    We don't want to set the current state to blocking
    as this is a fire and forget event.
    */  
    sound_service_dbus_update_sound_state (priv->service,
                                           BLOCKED);  
  }
}

gint
active_sink_get_index (ActiveSink* self)
{
  ActiveSinkPrivate* priv = ACTIVE_SINK_GET_PRIVATE (self);
  return slider_menu_item_get_sink_index (priv->volume_slider_menuitem);
}

gboolean
active_sink_is_populated (ActiveSink* sink)
{
  ActiveSinkPrivate* priv = ACTIVE_SINK_GET_PRIVATE (sink);
  return dbusmenu_menuitem_property_get_bool (DBUSMENU_MENUITEM (priv->volume_slider_menuitem),
                                                                 DBUSMENU_MENUITEM_PROP_ENABLED);
}

void 
active_sink_deactivate (ActiveSink* self)
{
  ActiveSinkPrivate* priv = ACTIVE_SINK_GET_PRIVATE (self);
  priv->current_sound_state = UNAVAILABLE;
  sound_service_dbus_update_sound_state (priv->service,
                                         priv->current_sound_state);  
  mute_menu_item_enable (priv->mute_menuitem, FALSE);
  slider_menu_item_enable (priv->volume_slider_menuitem, FALSE);
}

SoundState
active_sink_get_state (ActiveSink* self)
{
  ActiveSinkPrivate* priv = ACTIVE_SINK_GET_PRIVATE (self);
  return priv->current_sound_state;
}

void
active_sink_update_voip_input_source (ActiveSink* self, const pa_source_info* update)
{
  ActiveSinkPrivate* priv = ACTIVE_SINK_GET_PRIVATE (self);
  voip_input_menu_item_update (priv->voip_input_menu_item, update);
}

gboolean
active_sink_is_voip_source_populated (ActiveSink* self)
{
  ActiveSinkPrivate* priv = ACTIVE_SINK_GET_PRIVATE (self);
  return voip_input_menu_item_is_populated (priv->voip_input_menu_item);
}

gint active_sink_get_source_index (ActiveSink* self)
{
  ActiveSinkPrivate* priv = ACTIVE_SINK_GET_PRIVATE (self);
  return voip_input_menu_item_get_index (priv->voip_input_menu_item);
}

ActiveSink*
active_sink_new (SoundServiceDbus* service)
{
  ActiveSink* sink = g_object_new (ACTIVE_SINK_TYPE, NULL);
  ActiveSinkPrivate* priv = ACTIVE_SINK_GET_PRIVATE (sink);
  priv->service = service;
  sound_service_dbus_build_sound_menu (service,
                                       mute_menu_item_get_button (priv->mute_menuitem),
                                       DBUSMENU_MENUITEM (priv->volume_slider_menuitem),
                                       DBUSMENU_MENUITEM (priv->voip_input_menu_item));
  pm_establish_pulse_connection (sink);
  return sink; 
}
