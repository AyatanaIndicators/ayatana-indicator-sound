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
#include "scrub-widget.h"
#include "common-defs.h"

typedef struct _ScrubWidgetPrivate ScrubWidgetPrivate;

struct _ScrubWidgetPrivate
{
	DbusmenuMenuitem* twin_item;	
};

#define SCRUB_WIDGET_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), SCRUB_WIDGET_TYPE, ScrubWidgetPrivate))

/* Prototypes */
static void scrub_widget_class_init (ScrubWidgetClass *klass);
static void scrub_widget_init       (ScrubWidget *self);
static void scrub_widget_dispose    (GObject *object);
static void scrub_widget_finalize   (GObject *object);

static void scrub_widget_property_update(DbusmenuMenuitem* item, gchar* property, 
                                       GValue* value, gpointer userdata);
static void scrub_widget_set_twin_item(	ScrubWidget* self,
                           							DbusmenuMenuitem* twin_item);

G_DEFINE_TYPE (ScrubWidget, scrub_widget, G_TYPE_OBJECT);

static void
scrub_widget_class_init (ScrubWidgetClass *klass)
{
	GObjectClass 			*gobject_class = G_OBJECT_CLASS (klass);
	
	g_type_class_add_private (klass, sizeof (ScrubWidgetPrivate));

	gobject_class->dispose = scrub_widget_dispose;
	gobject_class->finalize = scrub_widget_finalize;
}

static void
scrub_widget_init (ScrubWidget *self)
{
	g_debug("ScrubWidget::scrub_widget_init");
	//ScrubWidgetPrivate * priv = SCRUB_WIDGET_GET_PRIVATE(self);
}

static void
scrub_widget_dispose (GObject *object)
{
	G_OBJECT_CLASS (scrub_widget_parent_class)->dispose (object);
}

static void
scrub_widget_finalize (GObject *object)
{
	G_OBJECT_CLASS (scrub_widget_parent_class)->finalize (object);
}

static void 
scrub_widget_property_update(DbusmenuMenuitem* item, gchar* property, 
                                       GValue* value, gpointer userdata)
{
	g_return_if_fail (IS_SCRUB_WIDGET (userdata));	
	//ScrubWidget* mitem = SCRUB_WIDGET(userdata);
	//ScrubWidgetPrivate * priv = SCRUB_WIDGET_GET_PRIVATE(mitem);
	
	if(g_ascii_strcasecmp(DBUSMENU_SCRUB_MENUITEM_DURATION, property) == 0){  
	}
	else if(g_ascii_strcasecmp(DBUSMENU_SCRUB_MENUITEM_POSITION, property) == 0){  
	}	
}

static void
scrub_widget_set_twin_item(ScrubWidget* self,
                           DbusmenuMenuitem* twin_item)
{
	ScrubWidgetPrivate * priv = SCRUB_WIDGET_GET_PRIVATE(self);
	priv->twin_item = twin_item;

	g_signal_connect(G_OBJECT(twin_item), "property-changed", 
	                 G_CALLBACK(scrub_widget_property_update), self);		
}
                           
 /**
 * scrub_widget_new:
 * @returns: a new #ScrubWidget.
 **/
GtkWidget* 
scrub_widget_new(DbusmenuMenuitem *item)
{
	GtkWidget* widget = g_object_new(SCRUB_WIDGET_TYPE, NULL);
	scrub_widget_set_twin_item((ScrubWidget*)widget, item);
	return widget;
}

