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

#include "device.h"
#include "slider-menu-item.h"
#include "mute-menu-item.h"
#include "voip-input-menu-item.h"
#include "pulseaudio-mgr.h"
#include "sound-state.h"

typedef struct _DevicePrivate DevicePrivate;

struct _DevicePrivate
{
  SliderMenuItem*     volume_slider_menuitem;
  MuteMenuItem*       mute_menuitem;
  VoipInputMenuItem*  voip_input_menu_item;
  SoundState          current_sound_state;
  SoundServiceDbus*   service;
};

#define DEVICE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), DEVICE_TYPE, DevicePrivate))

/* Prototypes */
static void device_class_init (DeviceClass *klass);
static void device_init       (Device *self);
static void device_dispose    (GObject *object);
static void device_finalize   (GObject *object);

static SoundState device_get_state_from_volume (Device* self);
static void device_mute_update (Device* self, gboolean muted);

G_DEFINE_TYPE (Device, device, G_TYPE_OBJECT);

static void
device_class_init (DeviceClass *klass)
{
  GObjectClass      *gobject_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (DevicePrivate));

  gobject_class->dispose = device_dispose;
  gobject_class->finalize = device_finalize;
}

static void
device_init (Device *self)
{
  DevicePrivate* priv = DEVICE_GET_PRIVATE (self);
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
device_dispose (GObject *object)
{  
  G_OBJECT_CLASS (device_parent_class)->dispose (object);
}

static void
device_finalize (GObject *object)
{
  G_OBJECT_CLASS (device_parent_class)->finalize (object);
}

void
device_sink_populate (Device* self,
                      const pa_sink_info* update)
{
  DevicePrivate* priv = DEVICE_GET_PRIVATE(self);
  mute_menu_item_enable (priv->mute_menuitem, TRUE);
  slider_menu_item_populate (priv->volume_slider_menuitem, update);
  SoundState state = device_get_state_from_volume (self);
  if (priv->current_sound_state != state){
    priv->current_sound_state  = state;
    sound_service_dbus_update_sound_state (priv->service,
                                           priv->current_sound_state);
  }
  device_mute_update (self, update->mute);
}

void
device_sink_update (Device* self,
                    const pa_sink_info* update)
{
  DevicePrivate* priv = DEVICE_GET_PRIVATE (self);
  slider_menu_item_update (priv->volume_slider_menuitem, update);

  SoundState state = device_get_state_from_volume (self);
  if (priv->current_sound_state != state){
    priv->current_sound_state  = state;
    sound_service_dbus_update_sound_state (priv->service,
                                           priv->current_sound_state);
  }

  device_mute_update (self, update->mute);
}

gint
device_get_voip_source_output_index (Device* self)
{
  DevicePrivate* priv = DEVICE_GET_PRIVATE (self);
  return voip_input_menu_item_get_source_output_index (priv->voip_input_menu_item);
}

static void 
device_mute_update (Device* self, gboolean muted)
{
  DevicePrivate* priv = DEVICE_GET_PRIVATE (self);
  mute_menu_item_update (priv->mute_menuitem, muted);
  SoundState state = device_get_state_from_volume (self);
  
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
device_ensure_sink_is_unmuted (Device* self)
{
  DevicePrivate* priv = DEVICE_GET_PRIVATE (self);
  if (mute_menu_item_is_muted (priv->mute_menuitem)){
    pm_update_mute (FALSE);    
  }
}


static SoundState
device_get_state_from_volume (Device* self)
{
  DevicePrivate* priv = DEVICE_GET_PRIVATE (self);
  GVariant* v = dbusmenu_menuitem_property_get_variant (DBUSMENU_MENUITEM(priv->volume_slider_menuitem),
                                                        DBUSMENU_VOLUME_MENUITEM_LEVEL);
  gdouble volume_percent = g_variant_get_double (v);

  return sound_state_get_from_volume ((int)volume_percent);
}

void
device_determine_blocking_state (Device* self)
{
  DevicePrivate* priv = DEVICE_GET_PRIVATE (self);
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
device_get_sink_index (Device* self)
{
  DevicePrivate* priv = DEVICE_GET_PRIVATE (self);
  return slider_menu_item_get_sink_index (priv->volume_slider_menuitem);
}

gboolean
device_is_sink_populated (Device* self)
{
  DevicePrivate* priv = DEVICE_GET_PRIVATE (self);
  return dbusmenu_menuitem_property_get_bool (DBUSMENU_MENUITEM (priv->volume_slider_menuitem),
                                                                 DBUSMENU_MENUITEM_PROP_ENABLED);
}

void
device_activate_voip_item (Device* self, gint source_output_index, gint client_index)
{
  DevicePrivate* priv = DEVICE_GET_PRIVATE (self);
  if (voip_input_menu_item_is_interested (priv->voip_input_menu_item,
                                          source_output_index,
                                          client_index)){
    voip_input_menu_item_enable (priv->voip_input_menu_item, TRUE);
  }
}

void
device_deactivate_voip_source (Device* self, gboolean visible)
{
  DevicePrivate* priv = DEVICE_GET_PRIVATE (self);
  visible &= voip_input_menu_item_is_active (priv->voip_input_menu_item);
  voip_input_menu_item_deactivate_source (priv->voip_input_menu_item, visible);
}

void
device_deactivate_voip_client (Device* self)
{
  DevicePrivate* priv = DEVICE_GET_PRIVATE (self);
  voip_input_menu_item_deactivate_voip_client (priv->voip_input_menu_item);
}

void 
device_sink_deactivated (Device* self)
{
  DevicePrivate* priv = DEVICE_GET_PRIVATE (self);
  priv->current_sound_state = UNAVAILABLE;
  sound_service_dbus_update_sound_state (priv->service,
                                         priv->current_sound_state);  
  mute_menu_item_enable (priv->mute_menuitem, FALSE);
  slider_menu_item_enable (priv->volume_slider_menuitem, FALSE);
}

SoundState
device_get_state (Device* self)
{
  DevicePrivate* priv = DEVICE_GET_PRIVATE (self);
  return priv->current_sound_state;
}

void
device_update_voip_input_source (Device* self, const pa_source_info* update)
{
  DevicePrivate* priv = DEVICE_GET_PRIVATE (self);
  voip_input_menu_item_update (priv->voip_input_menu_item, update);
}

gboolean
device_is_voip_source_populated (Device* self)
{
  DevicePrivate* priv = DEVICE_GET_PRIVATE (self);
  return voip_input_menu_item_is_populated (priv->voip_input_menu_item);
}

gint device_get_source_index (Device* self)
{
  DevicePrivate* priv = DEVICE_GET_PRIVATE (self);
  return voip_input_menu_item_get_index (priv->voip_input_menu_item);
}

Device*
device_new (SoundServiceDbus* service)
{
  Device* sink = g_object_new (DEVICE_TYPE, NULL);
  DevicePrivate* priv = DEVICE_GET_PRIVATE (sink);
  priv->service = service;
  sound_service_dbus_build_sound_menu (service,
                                       mute_menu_item_get_button (priv->mute_menuitem),
                                       DBUSMENU_MENUITEM (priv->volume_slider_menuitem),
                                       DBUSMENU_MENUITEM (priv->voip_input_menu_item));
  pm_establish_pulse_connection (sink);
  return sink; 
}
