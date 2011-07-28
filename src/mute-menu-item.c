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

#include "common-defs.h"
#include "mute-menu-item.h"
#include "pulseaudio-mgr.h"

typedef struct _MuteMenuItemPrivate MuteMenuItemPrivate;

struct _MuteMenuItemPrivate {
  DbusmenuMenuitem* button;
};

#define MUTE_MENU_ITEM_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), MUTE_MENU_ITEM_TYPE, MuteMenuItemPrivate))

/* Prototypes */
static void mute_menu_item_class_init (MuteMenuItemClass *klass);
static void mute_menu_item_init       (MuteMenuItem *self);
static void mute_menu_item_dispose    (GObject *object);
static void mute_menu_item_finalize   (GObject *object);
static void mute_menu_item_set_global_mute_from_ui (gpointer user_data);

G_DEFINE_TYPE (MuteMenuItem, mute_menu_item, G_TYPE_OBJECT);

static void
mute_menu_item_class_init (MuteMenuItemClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MuteMenuItemPrivate));

  object_class->dispose = mute_menu_item_dispose;
  object_class->finalize = mute_menu_item_finalize;

  return;
}

static void
mute_menu_item_init (MuteMenuItem *self)
{
  g_debug("Building new Mute Menu Item");
  MuteMenuItemPrivate* priv = MUTE_MENU_ITEM_GET_PRIVATE(self);
  priv->button = NULL;
  priv->button = dbusmenu_menuitem_new();

  dbusmenu_menuitem_property_set(priv->button,
                                 DBUSMENU_MENUITEM_PROP_TYPE,
                                 DBUSMENU_MUTE_MENUITEM_TYPE);

  dbusmenu_menuitem_property_set_bool (priv->button,
                                       DBUSMENU_MENUITEM_PROP_VISIBLE,
                                       TRUE);

  g_signal_connect (G_OBJECT (priv->button), 
                    DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,
                    G_CALLBACK (mute_menu_item_set_global_mute_from_ui),
                    self);  
  return;
}

static void
mute_menu_item_dispose (GObject *object)
{
  G_OBJECT_CLASS (mute_menu_item_parent_class)->dispose (object);
  return;
}

static void
mute_menu_item_finalize (GObject *object)
{
  G_OBJECT_CLASS (mute_menu_item_parent_class)->finalize (object);
}

static void 
mute_menu_item_set_global_mute_from_ui (gpointer user_data)
{
  g_return_if_fail (DBUSMENU_IS_MENUITEM (user_data));
  DbusmenuMenuitem* button = DBUSMENU_MENUITEM (user_data);
  gboolean current_value = dbusmenu_menuitem_property_get_bool (button,
                                                                DBUSMENU_MUTE_MENUITEM_VALUE);
  gboolean new_value = !current_value;
  pm_update_mute (new_value);
}

void
mute_menu_item_update (MuteMenuItem* item, gboolean value_update)
{
  MuteMenuItemPrivate* priv = MUTE_MENU_ITEM_GET_PRIVATE (item);
  
  dbusmenu_menuitem_property_set_bool (priv->button,
                                       DBUSMENU_MUTE_MENUITEM_VALUE,
                                       value_update);
  dbusmenu_menuitem_property_set (priv->button,
                                  DBUSMENU_MENUITEM_PROP_LABEL,
                                  value_update == FALSE ? _("Mute") : _("Unmute"));  
}

void
mute_menu_item_enable (MuteMenuItem* item, gboolean active)
{
  MuteMenuItemPrivate* priv = MUTE_MENU_ITEM_GET_PRIVATE (item);
  dbusmenu_menuitem_property_set_bool (priv->button,
                                       DBUSMENU_MENUITEM_PROP_VISIBLE,
                                       TRUE);
  
  dbusmenu_menuitem_property_set_bool (priv->button,
                                       DBUSMENU_MENUITEM_PROP_ENABLED,
                                       active);  
}

DbusmenuMenuitem*
mute_menu_item_get_button (MuteMenuItem* item)
{
  MuteMenuItemPrivate* priv = MUTE_MENU_ITEM_GET_PRIVATE (item);
  return priv->button;
}

gboolean
mute_menu_item_is_muted (MuteMenuItem* item)
{
  MuteMenuItemPrivate* priv = MUTE_MENU_ITEM_GET_PRIVATE (item);
  return dbusmenu_menuitem_property_get_bool (priv->button,
                                              DBUSMENU_MUTE_MENUITEM_VALUE);
}

MuteMenuItem*
mute_menu_item_new (gboolean initial_update, gboolean enabled)
{ 
  MuteMenuItem *self = g_object_new (MUTE_MENU_ITEM_TYPE, NULL);
  mute_menu_item_update (self, initial_update);
  mute_menu_item_enable (self, enabled);
  return self;
}
