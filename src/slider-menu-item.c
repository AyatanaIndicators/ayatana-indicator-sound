/*
Copyright 2010 Canonical Ltd.

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
#include "slider-menu-item.h"
#include "common-defs.h"
#include "pulseaudio-mgr.h"

typedef struct _SliderMenuItemPrivate SliderMenuItemPrivate;

struct _SliderMenuItemPrivate {
  Device*             a_sink;
  gint                index;
  gchar*              name;
  gboolean            mute;
  pa_cvolume          volume;
  pa_channel_map      channel_map;
  pa_volume_t         base_volume;
};

#define SLIDER_MENU_ITEM_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), SLIDER_MENU_ITEM_TYPE, SliderMenuItemPrivate))

/* Prototypes */
static void slider_menu_item_class_init (SliderMenuItemClass *klass);
static void slider_menu_item_init       (SliderMenuItem *self);
static void slider_menu_item_dispose    (GObject *object);
static void slider_menu_item_finalize   (GObject *object);
static void handle_event (DbusmenuMenuitem * mi, const gchar * name, 
                          GVariant * value, guint timestamp);
static pa_cvolume slider_menu_item_construct_mono_volume (const pa_cvolume* vol);
static void slider_menu_item_update_volume (SliderMenuItem* self, gdouble percent);

G_DEFINE_TYPE (SliderMenuItem, slider_menu_item, DBUSMENU_TYPE_MENUITEM);

static void
slider_menu_item_class_init (SliderMenuItemClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (SliderMenuItemPrivate));

  object_class->dispose = slider_menu_item_dispose;
  object_class->finalize = slider_menu_item_finalize;

  DbusmenuMenuitemClass * mclass = DBUSMENU_MENUITEM_CLASS(klass);
  mclass->handle_event = handle_event;
  return;
}

static void
slider_menu_item_init (SliderMenuItem *self)
{
  g_debug("Building new Slider Menu Item");
  dbusmenu_menuitem_property_set( DBUSMENU_MENUITEM(self),
                                  DBUSMENU_MENUITEM_PROP_TYPE,
                                  DBUSMENU_VOLUME_MENUITEM_TYPE );

  SliderMenuItemPrivate* priv = SLIDER_MENU_ITEM_GET_PRIVATE (self);

  priv->index = NOT_ACTIVE;
  priv->name = NULL;

  return;
}

static void
slider_menu_item_dispose (GObject *object)
{
  G_OBJECT_CLASS (slider_menu_item_parent_class)->dispose (object);
  return;
}

static void
slider_menu_item_finalize (GObject *object)
{
  G_OBJECT_CLASS (slider_menu_item_parent_class)->finalize (object);
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

  if (value != NULL){
    if (IS_SLIDER_MENU_ITEM (mi)) {
      SliderMenuItemPrivate* priv = SLIDER_MENU_ITEM_GET_PRIVATE (SLIDER_MENU_ITEM (mi));
      gdouble volume_input = g_variant_get_double(input);
      //g_debug ("slider menu item about to update volume %f", volume_input);
      slider_menu_item_update_volume (SLIDER_MENU_ITEM (mi), volume_input);
      device_ensure_sink_is_unmuted (priv->a_sink);
    }    
  }
}


void
slider_menu_item_populate (SliderMenuItem* self, const pa_sink_info* update)
{
  SliderMenuItemPrivate* priv = SLIDER_MENU_ITEM_GET_PRIVATE (self);
  priv->name = g_strdup (update->name);
  priv->index = update->index;
  priv->volume = slider_menu_item_construct_mono_volume (&update->volume);
  priv->base_volume = update->base_volume;
  priv->channel_map = update->channel_map;
  priv->mute = update->mute;

  pa_volume_t vol = pa_cvolume_max (&update->volume);
  gdouble volume_percent = ((gdouble) vol * 100) / PA_VOLUME_NORM;
  GVariant* new_volume = g_variant_new_double (volume_percent);
  dbusmenu_menuitem_property_set_variant (DBUSMENU_MENUITEM(self),
                                          DBUSMENU_VOLUME_MENUITEM_LEVEL,
                                          new_volume);
  GVariant* new_mute_update = g_variant_new_int32 (update->mute);
  dbusmenu_menuitem_property_set_variant (DBUSMENU_MENUITEM(self),
                                          DBUSMENU_VOLUME_MENUITEM_MUTE,
                                          new_mute_update);

  slider_menu_item_enable (self, TRUE);
}

// From the UI
static void
slider_menu_item_update_volume (SliderMenuItem* self, gdouble percent)
{
  pa_cvolume new_volume;
  pa_cvolume_init(&new_volume);
  new_volume.channels = 1;
  pa_volume_t new_volume_value = (pa_volume_t) ((percent * PA_VOLUME_NORM) / 100);
  pa_cvolume_set(&new_volume, 1, new_volume_value);

  SliderMenuItemPrivate* priv = SLIDER_MENU_ITEM_GET_PRIVATE (self);

  pa_cvolume_set(&priv->volume, priv->channel_map.channels, new_volume_value);
  pm_update_volume (priv->index, new_volume);
}

// To the UI
void
slider_menu_item_update (SliderMenuItem* self, const pa_sink_info* update)
{
  SliderMenuItemPrivate* priv = SLIDER_MENU_ITEM_GET_PRIVATE (self);

  priv->volume = slider_menu_item_construct_mono_volume (&update->volume);
  priv->base_volume = update->base_volume;
  priv->channel_map = update->channel_map;

  pa_volume_t vol = pa_cvolume_max (&update->volume);
  gdouble volume_percent = ((gdouble) vol * 100) / PA_VOLUME_NORM;

  GVariant* new_volume = g_variant_new_double (volume_percent);
  dbusmenu_menuitem_property_set_variant (DBUSMENU_MENUITEM(self),
                                          DBUSMENU_VOLUME_MENUITEM_LEVEL,
                                          new_volume);
  if (priv->mute != update->mute){
    priv->mute = update->mute;
    g_debug ("volume menu item - update - mute = %i", update->mute);
    GVariant* new_mute_update = g_variant_new_int32 (update->mute);
    dbusmenu_menuitem_property_set_variant (DBUSMENU_MENUITEM(self),
                                            DBUSMENU_VOLUME_MENUITEM_MUTE,
                                            new_mute_update);
  }
}

/*
 * Enable/Disabled can be considered the equivalent of whether we have an active
 * sink or not, let the widget have inherent state.
 */
void
slider_menu_item_enable (SliderMenuItem* self, gboolean active)
{
  SliderMenuItemPrivate* priv = SLIDER_MENU_ITEM_GET_PRIVATE (self);

  dbusmenu_menuitem_property_set_bool (DBUSMENU_MENUITEM(self),
                                       DBUSMENU_MENUITEM_PROP_ENABLED,
                                       active);
  if(active == FALSE){
    priv->index = NOT_ACTIVE;
    if(priv->name != NULL){
      g_free(priv->name);
      priv->name = NULL;
    }
  }
}

gint
slider_menu_item_get_sink_index (SliderMenuItem* self)
{
  SliderMenuItemPrivate* priv = SLIDER_MENU_ITEM_GET_PRIVATE (self);
  return priv->index;
}

static pa_cvolume
slider_menu_item_construct_mono_volume (const pa_cvolume* vol)
{
  pa_cvolume new_volume;
  pa_cvolume_init(&new_volume);
  new_volume.channels = 1;
  pa_volume_t max_vol = pa_cvolume_max(vol);
  pa_cvolume_set(&new_volume, 1, max_vol);
  return new_volume;
}

SliderMenuItem*
slider_menu_item_new (Device* sink)
{ 
  SliderMenuItem *self = g_object_new(SLIDER_MENU_ITEM_TYPE, NULL);
  SliderMenuItemPrivate* priv = SLIDER_MENU_ITEM_GET_PRIVATE (self);
  priv->a_sink = sink;
  return self;
}