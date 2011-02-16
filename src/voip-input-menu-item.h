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
#ifndef __VOIP_INPUT_MENU_ITEM_H__
#define __VOIP_INPUT_MENU_ITEM_H__

#include <glib.h>
#include <pulse/pulseaudio.h>
#include <libdbusmenu-glib/menuitem.h>
#include "active-sink.h"

G_BEGIN_DECLS

#define VOIP_INPUT_MENU_ITEM_TYPE            (voip_input_menu_item_get_type ())
#define VOIP_INPUT_MENU_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), VOIP_INPUT_MENU_ITEM_TYPE, VoipInputMenuItem))
#define VOIP_INPUT_MENU_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), VOIP_INPUT_MENU_ITEM_TYPE, VoipInputMenuItemClass))
#define IS_VOIP_INPUT_MENU_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VOIP_INPUT_MENU_ITEM_TYPE))
#define IS_VOIP_INPUT_MENU_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), VOIP_INPUT_MENU_ITEM_TYPE))
#define VOIP_INPUT_MENU_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), VOIP_INPUT_MENU_ITEM_TYPE, VoipInputMenuItemClass))

typedef struct _VoipInputMenuItem      VoipInputMenuItem;
typedef struct _VoipInputMenuItemClass VoipInputMenuItemClass;

struct _VoipInputMenuItemClass {
  DbusmenuMenuitemClass parent_class;
};

struct _VoipInputMenuItem {
  DbusmenuMenuitem parent;
};

GType voip_input_menu_item_get_type (void);

void voip_input_menu_item_update (VoipInputMenuItem* item,
                                  const pa_source_info* source);
void voip_input_menu_item_update_source_details (VoipInputMenuItem* item,
                                                 const pa_source_info* source);

void voip_input_menu_item_enable (VoipInputMenuItem* item, gboolean active);

VoipInputMenuItem* voip_input_menu_item_new (ActiveSink* sink);

G_END_DECLS

#endif

