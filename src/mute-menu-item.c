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

#include "common-defs.h"
#include <glib/gi18n.h>
#include "mute-menu-item.h"
#include "pulse-manager.h"

typedef struct _MuteMenuItemPrivate MuteMenuItemPrivate;

struct _MuteMenuItemPrivate {
};

#define MUTE_MENU_ITEM_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), MUTE_MENU_ITEM_TYPE, MuteMenuItemPrivate))

/* Prototypes */
static void mute_menu_item_class_init (MuteMenuItemClass *klass);
static void mute_menu_item_init       (MuteMenuItem *self);
static void mute_menu_item_dispose    (GObject *object);
static void mute_menu_item_finalize   (GObject *object);
static void handle_event (DbusmenuMenuitem * mi, const gchar * name, 
                          GVariant * value, guint timestamp);

G_DEFINE_TYPE (MuteMenuItem, mute_menu_item, DBUSMENU_TYPE_MENUITEM);

static void mute_menu_item_class_init (MuteMenuItemClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MuteMenuItemPrivate));

  object_class->dispose = mute_menu_item_dispose;
  object_class->finalize = mute_menu_item_finalize;

  DbusmenuMenuitemClass * mclass = DBUSMENU_MENUITEM_CLASS(klass);
  mclass->handle_event = handle_event;
  return;
}

static void mute_menu_item_init (MuteMenuItem *self)
{
  g_debug("Building new Mute Menu Item");
  return;
}

static void mute_menu_item_dispose (GObject *object)
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
handle_event (DbusmenuMenuitem * mi,
              const gchar * name,
              GVariant * value,
              guint timestamp)
{
  /*g_debug ( "handle-event in the mute at the backend, input is of type %s",
             g_variant_get_type_string(value));*/

  GVariant* input = NULL;
  input = value;
  g_variant_ref (input);

  // Please note: Subject to change in future DBusmenu revisions
  if (g_variant_is_of_type(value, G_VARIANT_TYPE_VARIANT) == TRUE) {
    input = g_variant_get_variant(value);
  }
  
  gboolean mute_input = g_variant_get_boolean(input);
  // TODO: use the pulse wrapper directly
  toggle_global_mute (mute_input);
  g_variant_unref (input); 
}

void mute_menu_item_update(MuteMenuItem* item, gboolean value_update)
{
  dbusmenu_menuitem_property_set_bool (DBUSMENU_MENUITEM(item),
                                       DBUSMENU_MUTE_MENUITEM_VALUE,
                                       value_update);
  dbusmenu_menuitem_property_set (DBUSMENU_MENUITEM(item),
                                  DBUSMENU_MENUITEM_PROP_LABEL,
                                  value_update == FALSE ? _("Mute") : _("Unmute"));  
}

void mute_menu_item_enable(MuteMenuItem* item, gboolean active)
{
  dbusmenu_menuitem_property_set_bool(DBUSMENU_MENUITEM(item),
                                      DBUSMENU_MENUITEM_PROP_ENABLED,
                                      active);  
}


MuteMenuItem* mute_menu_item_new (gboolean initial_update, gboolean enabled)
{ 
  MuteMenuItem *self = g_object_new(MUTE_MENU_ITEM_TYPE, NULL);
  dbusmenu_menuitem_property_set (DBUSMENU_MENUITEM(self),
                                  DBUSMENU_MENUITEM_PROP_TYPE,
                                  DBUSMENU_MUTE_MENUITEM_TYPE);
  mute_menu_item_update (self, initial_update);
  mute_menu_item_enable (self, enabled);
  return self;
}
