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

#ifndef __MUTE_MENU_ITEM_H__
#define __MUTE_MENU_ITEM_H__

#include <glib.h>
#include <glib-object.h>
#include <libdbusmenu-glib/menuitem.h>

G_BEGIN_DECLS

#define MUTE_MENU_ITEM_TYPE            (mute_menu_item_get_type ())
#define MUTE_MENU_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MUTE_MENU_ITEM_TYPE, MuteMenuItem))
#define MUTE_MENU_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MUTE_MENU_ITEM_TYPE, MuteMenuItemClass))
#define IS_MUTE_MENU_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MUTE_MENU_ITEM_TYPE))
#define IS_MUTE_MENU_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MUTE_MENU_ITEM_TYPE))
#define MUTE_MENU_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MUTE_MENU_ITEM_TYPE, MuteMenuItemClass))

typedef struct _MuteMenuItem      MuteMenuItem;
typedef struct _MuteMenuItemClass MuteMenuItemClass;

struct _MuteMenuItemClass {
  GObjectClass parent_class;
};

struct _MuteMenuItem {
  GObject parent;
};

GType mute_menu_item_get_type (void);

MuteMenuItem* mute_menu_item_new ();

void mute_menu_item_update (MuteMenuItem* item, gboolean update);
void mute_menu_item_enable (MuteMenuItem* item, gboolean active);
gboolean mute_menu_item_is_muted (MuteMenuItem* item);

DbusmenuMenuitem* mute_menu_item_get_button (MuteMenuItem* item);

G_END_DECLS

#endif