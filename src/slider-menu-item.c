/*
Copyright 2010 Canonical Ltd.

Authors:
    Conor Curran <conor.curran@canonical.com>
    Ted Gould <ted@canonical.com>

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
#include "pulse-manager.h"
#include "common-defs.h"

/*enum {*/
/*	VOLUME_CHANGED,*/
/*	FINAL_SIGNAL*/
/*};*/

/*static guint slider_signals[FINAL_SIGNAL] = { 0 };*/

typedef struct _SliderMenuItemPrivate SliderMenuItemPrivate;

struct _SliderMenuItemPrivate
{
	gdouble slider_value;
};

#define SLIDER_MENU_ITEM_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), SLIDER_MENU_ITEM_TYPE, SliderMenuItemPrivate))

/* Prototypes */
static void slider_menu_item_class_init (SliderMenuItemClass *klass);
static void slider_menu_item_init       (SliderMenuItem *self);
static void slider_menu_item_dispose    (GObject *object);
static void slider_menu_item_finalize   (GObject *object);
static void handle_event (DbusmenuMenuitem * mi, const gchar * name, const GValue * value, guint timestamp);

G_DEFINE_TYPE (SliderMenuItem, slider_menu_item, DBUSMENU_TYPE_MENUITEM);

static void slider_menu_item_class_init (SliderMenuItemClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (SliderMenuItemPrivate));

	object_class->dispose = slider_menu_item_dispose;
	object_class->finalize = slider_menu_item_finalize;

    DbusmenuMenuitemClass * mclass = DBUSMENU_MENUITEM_CLASS(klass);
    mclass->handle_event = handle_event;
/*	signals[VOLUME_CHANGED] =  g_signal_new(SLIDER_MENU_ITEM_SIGNAL_VOLUME_CHANGED,*/
/*	                                      G_TYPE_FROM_CLASS(klass),*/
/*	                                      G_SIGNAL_RUN_LAST,*/
/*	                                      G_STRUCT_OFFSET (SliderMenuItemClass, volume_changed),*/
/*	                                      NULL, NULL,*/
/*	                                      g_cclosure_marshal_VOID__DOUBLE,*/
/*	                                      G_TYPE_NONE, 1, G_TYPE_DOUBLE);*/
	return;
}

static void slider_menu_item_init (SliderMenuItem *self)
{
	g_debug("Building new Slider Menu Item");
	return;
}

static void slider_menu_item_dispose (GObject *object)
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
handle_event (DbusmenuMenuitem * mi, const gchar * name, const GValue * value, guint timestamp)
{
	g_debug("in the handle event method of slider_menu_item");

    set_sink_volume((gdouble)g_value_get_double(value));
}



SliderMenuItem* slider_menu_item_new(gboolean sinks_available, gdouble start_volume)
{
	SliderMenuItem *self = g_object_new(SLIDER_MENU_ITEM_TYPE, NULL);
	//SliderMenuItemPrivate * priv = SLIDER_MENU_ITEM_GET_PRIVATE(self);
    dbusmenu_menuitem_property_set(DBUSMENU_MENUITEM(self), DBUSMENU_MENUITEM_PROP_TYPE, DBUSMENU_SLIDER_MENUITEM_TYPE);
    //TODO: If no valid sinks are found grey out the item(s)
    dbusmenu_menuitem_property_set_bool(DBUSMENU_MENUITEM(self), DBUSMENU_MENUITEM_PROP_SENSITIVE, sinks_available);

    GValue value = {0};
    g_value_init(&value, G_TYPE_DOUBLE);
    g_value_set_double(&value, start_volume);
    //dbusmenu_menuitem_property_set_value(DBUSMENU_MENUITEM(self), DBUSMENU_SLIDER_MENUITEM_PROP_VOLUME, &value);
	return self;
}




