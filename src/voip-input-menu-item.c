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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib/gi18n.h>
#include "voip-input-menu-item.h"
#include "common-defs.h"
#include "pulseaudio-mgr.h"

typedef struct _VoipInputMenuItemPrivate VoipInputMenuItemPrivate;

struct _VoipInputMenuItemPrivate {
  Device*             a_sink;
  pa_cvolume          volume;
  gint                mute;
  guint32             volume_steps;
  pa_channel_map      channel_map;
  pa_volume_t         base_volume;
  gint                source_index;
  gint                source_output_index;
  gint                client_index;
};

#define VOIP_INPUT_MENU_ITEM_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), VOIP_INPUT_MENU_ITEM_TYPE, VoipInputMenuItemPrivate))

/* Prototypes */
static void voip_input_menu_item_class_init (VoipInputMenuItemClass *klass);
static void voip_input_menu_item_init       (VoipInputMenuItem *self);
static void voip_input_menu_item_dispose    (GObject *object);
static void voip_input_menu_item_finalize   (GObject *object);
static void handle_event (DbusmenuMenuitem * mi, const gchar * name,
                          GVariant * value, guint timestamp);
// TODO:
// This method should really be shared between this and the volume slider obj
// perfectly static - wait until the device mgr wrapper is properly sorted and
// then consolidate
static pa_cvolume voip_input_menu_item_construct_mono_volume (const pa_cvolume* vol);

G_DEFINE_TYPE (VoipInputMenuItem, voip_input_menu_item, DBUSMENU_TYPE_MENUITEM);

static void
voip_input_menu_item_class_init (VoipInputMenuItemClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (VoipInputMenuItemPrivate));

  object_class->dispose = voip_input_menu_item_dispose;
  object_class->finalize = voip_input_menu_item_finalize;

  DbusmenuMenuitemClass * mclass = DBUSMENU_MENUITEM_CLASS(klass);
  mclass->handle_event = handle_event;
}

static void
voip_input_menu_item_init (VoipInputMenuItem *self)
{
  dbusmenu_menuitem_property_set( DBUSMENU_MENUITEM(self),
                                  DBUSMENU_MENUITEM_PROP_TYPE,
                                  DBUSMENU_VOIP_INPUT_MENUITEM_TYPE );
  VoipInputMenuItemPrivate* priv = VOIP_INPUT_MENU_ITEM_GET_PRIVATE (self);
  dbusmenu_menuitem_property_set_bool( DBUSMENU_MENUITEM(self),
                                       DBUSMENU_MENUITEM_PROP_VISIBLE,
                                       FALSE );

  priv->source_index     = NOT_ACTIVE;
  priv->source_output_index = NOT_ACTIVE;
  priv->client_index     = NOT_ACTIVE;
  priv->mute             = NOT_ACTIVE;
}

static void
voip_input_menu_item_dispose (GObject *object)
{
  G_OBJECT_CLASS (voip_input_menu_item_parent_class)->dispose (object);
  return;
}

static void
voip_input_menu_item_finalize (GObject *object)
{
  G_OBJECT_CLASS (voip_input_menu_item_parent_class)->finalize (object);
}

static void
handle_event (DbusmenuMenuitem * mi,
              const gchar * name,
              GVariant * value,
              guint timestamp)
{
  GVariant* input = NULL;
  input = value;
  if (g_variant_is_of_type(value, G_VARIANT_TYPE_VARIANT) == TRUE) {
    input = g_variant_get_variant(value);
  }

  gdouble percent = g_variant_get_double(input);
  if (value != NULL){
    if (IS_VOIP_INPUT_MENU_ITEM (mi)) {
      VoipInputMenuItemPrivate* priv = VOIP_INPUT_MENU_ITEM_GET_PRIVATE (VOIP_INPUT_MENU_ITEM (mi));
/*
      g_debug ("Handle event in the voip input level backend instance - %f", percent);
*/
      pa_cvolume new_volume;
      pa_cvolume_init(&new_volume);
      new_volume.channels = 1;
      pa_volume_t new_volume_value = (pa_volume_t) ((percent * PA_VOLUME_NORM) / 100);
      pa_cvolume_set(&new_volume, 1, new_volume_value);

      pm_update_mic_gain (priv->source_index, new_volume);
      // finally unmute if needed
      if (priv->mute == 1) {
        pm_update_mic_mute (priv->source_index, 0);
      }
    }
  }
}

static pa_cvolume
voip_input_menu_item_construct_mono_volume (const pa_cvolume* vol)
{
  pa_cvolume new_volume;
  pa_cvolume_init(&new_volume);
  new_volume.channels = 1;
  pa_volume_t max_vol = pa_cvolume_max(vol);
  pa_cvolume_set(&new_volume, 1, max_vol);
  return new_volume;
}

void
voip_input_menu_item_update (VoipInputMenuItem* item,
                             const pa_source_info* source)
{
  VoipInputMenuItemPrivate* priv = VOIP_INPUT_MENU_ITEM_GET_PRIVATE (item);
  // only overwrite the constants of each source if the device has changed
  if (priv->source_index == NOT_ACTIVE){
    priv->base_volume = source->base_volume;
    priv->volume_steps = source->n_volume_steps;
    priv->channel_map = source->channel_map;
    priv->source_index = source->index;
  }
  priv->volume = voip_input_menu_item_construct_mono_volume (&source->volume);
  pa_volume_t vol = pa_cvolume_max (&source->volume);
  gdouble update = ((gdouble) vol * 100) / PA_VOLUME_NORM;
  
  GVariant* new_volume = g_variant_new_double(update);
  dbusmenu_menuitem_property_set_variant(DBUSMENU_MENUITEM(item),
                                         DBUSMENU_VOIP_INPUT_MENUITEM_LEVEL,
                                         new_volume);
  // Only send over the mute updates if the state has changed.
  // in this order - volume first mute last!!
  if (priv->mute != source->mute){
/*
    g_debug ("voip menu item - update - mute = %i", priv->mute);
*/
    GVariant* new_mute_update = g_variant_new_int32 (source->mute);
    dbusmenu_menuitem_property_set_variant (DBUSMENU_MENUITEM(item),
                                            DBUSMENU_VOIP_INPUT_MENUITEM_MUTE,
                                            new_mute_update);
  }

  priv->mute = source->mute;

}

gboolean
voip_input_menu_item_is_interested (VoipInputMenuItem* item,
                                    gint source_output_index,
                                    gint client_index)
{
  VoipInputMenuItemPrivate* priv = VOIP_INPUT_MENU_ITEM_GET_PRIVATE (item);
  // Check to make sure we are not handling another voip beforehand and that we
  // have an active sink (might need to match up at start up)
  if (priv->source_output_index != NOT_ACTIVE &&
      priv->source_index != NOT_ACTIVE){
    return FALSE;
  }
  
  priv->source_output_index = source_output_index;
  priv->client_index     = client_index;

  return TRUE;
}

gboolean
voip_input_menu_item_is_active (VoipInputMenuItem* item)
{
  VoipInputMenuItemPrivate* priv = VOIP_INPUT_MENU_ITEM_GET_PRIVATE (item);
  return (priv->source_output_index != NOT_ACTIVE && priv->client_index != NOT_ACTIVE);
}


gboolean
voip_input_menu_item_is_populated (VoipInputMenuItem* item)
{
  VoipInputMenuItemPrivate* priv = VOIP_INPUT_MENU_ITEM_GET_PRIVATE (item);
  return priv->source_index != NOT_ACTIVE;
}

gint
voip_input_menu_item_get_index (VoipInputMenuItem* item)
{
  VoipInputMenuItemPrivate* priv = VOIP_INPUT_MENU_ITEM_GET_PRIVATE (item);
  return priv->source_index;
}

gint
voip_input_menu_item_get_source_output_index (VoipInputMenuItem* item)
{
  VoipInputMenuItemPrivate* priv = VOIP_INPUT_MENU_ITEM_GET_PRIVATE (item);

  return priv->source_output_index;
}

/**
 * If the pulse server informs of a default source change
 * or the source in question is removed.
 * @param item
 */
void
voip_input_menu_item_deactivate_source (VoipInputMenuItem* item, gboolean visible)
{
  VoipInputMenuItemPrivate* priv = VOIP_INPUT_MENU_ITEM_GET_PRIVATE (item);
  priv->source_index = NOT_ACTIVE;
  dbusmenu_menuitem_property_set_bool( DBUSMENU_MENUITEM(item),
                                       DBUSMENU_MENUITEM_PROP_VISIBLE,
                                       visible );
}

void
voip_input_menu_item_deactivate_voip_client (VoipInputMenuItem* item)
{
  VoipInputMenuItemPrivate* priv = VOIP_INPUT_MENU_ITEM_GET_PRIVATE (item);
  priv->client_index = NOT_ACTIVE;
  priv->source_output_index = NOT_ACTIVE;
  voip_input_menu_item_enable (item, FALSE);
}

void
voip_input_menu_item_enable (VoipInputMenuItem* item,
                             gboolean active)
{
  VoipInputMenuItemPrivate* priv = VOIP_INPUT_MENU_ITEM_GET_PRIVATE (item);
  if (priv->source_index == NOT_ACTIVE && active == TRUE) {
    g_warning ("Tried to enable the VOIP menuitem but we don't have an active source ??");
    active = FALSE;
  }
  dbusmenu_menuitem_property_set_bool( DBUSMENU_MENUITEM(item),
                                       DBUSMENU_MENUITEM_PROP_VISIBLE,
                                       active );
}

VoipInputMenuItem*
voip_input_menu_item_new (Device* sink)
{
  VoipInputMenuItem *self = g_object_new(VOIP_INPUT_MENU_ITEM_TYPE, NULL);
  VoipInputMenuItemPrivate* priv = VOIP_INPUT_MENU_ITEM_GET_PRIVATE (self);
  priv->a_sink = sink;
  return self;
}