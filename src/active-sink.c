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
  gint                index;
  gchar*              name;
  pa_cvolume          volume;
  pa_channel_map      channel_map;
  pa_volume_t         base_volume;
};

#define ACTIVE_SINK_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), ACTIVE_SINK_TYPE, ActiveSinkPrivate))

/* Prototypes */
static void active_sink_class_init (ActiveSinkClass *klass);
static void active_sink_init       (ActiveSink *self);
static void active_sink_dispose    (GObject *object);
static void active_sink_finalize   (GObject *object);

static SoundState active_sink_get_state_from_volume (ActiveSink* self);
static pa_cvolume active_sink_construct_mono_volume (const pa_cvolume* vol);
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
  priv->index = -1;
  priv->name = NULL;
  priv->service = NULL;

  // Init our menu items.
  priv->mute_menuitem = g_object_new (MUTE_MENU_ITEM_TYPE, NULL);
  priv->voip_input_menu_item = g_object_new (VOIP_INPUT_MENU_ITEM_TYPE, NULL);;
  priv->volume_slider_menuitem = slider_menu_item_new (self);
  mute_menu_item_enable (priv->mute_menuitem, FALSE);
  slider_menu_item_enable (priv->volume_slider_menuitem, FALSE);  
}

void
active_sink_activate_voip_item (ActiveSink* self)
{
  ActiveSinkPrivate* priv = ACTIVE_SINK_GET_PRIVATE (self);
  voip_input_menu_item_enable (priv->voip_input_menu_item, TRUE);
}

void
active_sink_deactivate_voip_source (ActiveSink* self)
{
  ActiveSinkPrivate* priv = ACTIVE_SINK_GET_PRIVATE (self);
  voip_input_menu_item_enable (priv->voip_input_menu_item, FALSE);
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

  priv->name = g_strdup (update->name);
  priv->index = update->index;
  active_sink_mute_update (sink, update->mute);
  priv->volume = active_sink_construct_mono_volume (&update->volume);
  priv->base_volume = update->base_volume;
  priv->channel_map = update->channel_map;

  pa_volume_t vol = pa_cvolume_max (&update->volume);
  gdouble volume_percent = ((gdouble) vol * 100) / PA_VOLUME_NORM;

  active_sink_volume_update (sink, volume_percent);  
  active_sink_mute_update (sink, update->mute);
  mute_menu_item_enable (priv->mute_menuitem, TRUE);
  slider_menu_item_enable (priv->volume_slider_menuitem, TRUE);

  g_debug ("Active sink has been populated - volume %f", volume_percent);
}

void
active_sink_update (ActiveSink* sink,
                    const pa_sink_info* update)
{
  ActiveSinkPrivate* priv = ACTIVE_SINK_GET_PRIVATE (sink);
  active_sink_mute_update (sink, update->mute);
  priv->volume = active_sink_construct_mono_volume (&update->volume);
  priv->base_volume = update->base_volume;
  priv->channel_map = update->channel_map;

  pa_volume_t vol = pa_cvolume_max (&update->volume);
  gdouble volume_percent = ((gdouble) vol * 100) / PA_VOLUME_NORM;

  active_sink_volume_update (sink, volume_percent);  
  active_sink_mute_update (sink, update->mute);  
}

// To the UI
static void
active_sink_volume_update (ActiveSink* self, gdouble percent)
{
  ActiveSinkPrivate* priv = ACTIVE_SINK_GET_PRIVATE (self);
  slider_menu_item_update (priv->volume_slider_menuitem, percent);  
  SoundState state = active_sink_get_state_from_volume (self);
  if (priv->current_sound_state != state){
    priv->current_sound_state  = state;
    sound_service_dbus_update_sound_state (priv->service,
                                           priv->current_sound_state);
  }
}

// From the UI
void 
active_sink_update_volume (ActiveSink* self, gdouble percent)
{
  pa_cvolume new_volume;
  pa_cvolume_init(&new_volume);
  new_volume.channels = 1;
  pa_volume_t new_volume_value = (pa_volume_t) ((percent * PA_VOLUME_NORM) / 100);
  pa_cvolume_set(&new_volume, 1, new_volume_value);

  ActiveSinkPrivate* priv = ACTIVE_SINK_GET_PRIVATE (self);

  pa_cvolume_set(&priv->volume, priv->channel_map.channels, new_volume_value);
  pm_update_volume (priv->index, new_volume);
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

pa_cvolume
active_sink_construct_mono_volume (const pa_cvolume* vol)
{
  pa_cvolume new_volume;
  pa_cvolume_init(&new_volume);
  new_volume.channels = 1;
  pa_volume_t max_vol = pa_cvolume_max(vol);
  pa_cvolume_set(&new_volume, 1, max_vol);
  return new_volume;
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
  return priv->index;
}

gboolean
active_sink_is_populated (ActiveSink* sink)
{
  ActiveSinkPrivate* priv = ACTIVE_SINK_GET_PRIVATE (sink);
  return (priv->index != -1);
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
  priv->index = -1;
  g_free(priv->name);
  priv->name = NULL;
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
