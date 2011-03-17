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
#ifndef __SLIDER_MENU_ITEM_H__
#define __SLIDER_MENU_ITEM_H__

#include <glib.h>
#include <glib-object.h>

#include <libdbusmenu-glib/menuitem.h>
#include "device.h"

G_BEGIN_DECLS

#define SLIDER_MENU_ITEM_TYPE            (slider_menu_item_get_type ())
#define SLIDER_MENU_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SLIDER_MENU_ITEM_TYPE, SliderMenuItem))
#define SLIDER_MENU_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SLIDER_MENU_ITEM_TYPE, SliderMenuItemClass))
#define IS_SLIDER_MENU_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SLIDER_MENU_ITEM_TYPE))
#define IS_SLIDER_MENU_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SLIDER_MENU_ITEM_TYPE))
#define SLIDER_MENU_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), SLIDER_MENU_ITEM_TYPE, SliderMenuItemClass))

typedef struct _SliderMenuItem      SliderMenuItem;
typedef struct _SliderMenuItemClass SliderMenuItemClass;

struct _SliderMenuItemClass {
  DbusmenuMenuitemClass parent_class;
};

struct _SliderMenuItem {
  DbusmenuMenuitem parent;
};

GType slider_menu_item_get_type (void);

void slider_menu_item_update(SliderMenuItem* item, const pa_sink_info* update);
void slider_menu_item_enable(SliderMenuItem* item, gboolean active);
void slider_menu_item_populate (SliderMenuItem* self, const pa_sink_info* update);
//void
//active_sink_update (ActiveSink* sink,
//                    const pa_sink_info* update)

gint slider_menu_item_get_sink_index (SliderMenuItem* self);

SliderMenuItem* slider_menu_item_new (Device* sink);

G_END_DECLS

#endif

