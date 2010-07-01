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
#include "transport-widget.h"
#include "common-defs.h"
#include <gtk/gtk.h>
#include "play-button.h"


static DbusmenuMenuitem* twin_item;

typedef struct _TransportWidgetPrivate TransportWidgetPrivate;

struct _TransportWidgetPrivate
{
	GtkWidget* hbox;
	GtkWidget* play_button;
};

#define TRANSPORT_WIDGET_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TRANSPORT_WIDGET_TYPE, TransportWidgetPrivate))

/* Gobject boiler plate */
static void transport_widget_class_init (TransportWidgetClass *klass);
static void transport_widget_init       (TransportWidget *self);
static void transport_widget_dispose    (GObject *object);
static void transport_widget_finalize   (GObject *object);
static gboolean transport_widget_expose_event(GtkWidget* widget, GdkEventExpose* event);

/* UI and dbusmenu callbacks */
static gboolean transport_widget_button_press_event 	(GtkWidget             *menuitem,
                                                  		GdkEventButton        *event);
static gboolean transport_widget_button_release_event (GtkWidget             *menuitem,
                                                    	GdkEventButton        *event);                                          
static void transport_widget_property_update(DbusmenuMenuitem* item,
                                       				gchar * property, 
                                       				GValue * value,
                                       				gpointer userdata);

G_DEFINE_TYPE (TransportWidget, transport_widget, GTK_TYPE_MENU_ITEM);

static void
transport_widget_class_init (TransportWidgetClass *klass)
{
	GObjectClass 			*gobject_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass    *widget_class = GTK_WIDGET_CLASS (klass);
	GtkMenuItemClass *menu_item_class =  GTK_MENU_ITEM_CLASS(klass);

	menu_item_class->hide_on_activate = FALSE;
  widget_class->button_press_event = transport_widget_button_press_event;
  widget_class->button_release_event = transport_widget_button_release_event;	
	widget_class->expose_event = transport_widget_expose_event;
	g_type_class_add_private (klass, sizeof (TransportWidgetPrivate));

	gobject_class->dispose = transport_widget_dispose;
	gobject_class->finalize = transport_widget_finalize;
}

static void
transport_widget_init (TransportWidget *self)
{
	g_debug("TransportWidget::transport_widget_init");

	TransportWidgetPrivate * priv = TRANSPORT_WIDGET_GET_PRIVATE(self);
	GtkWidget* hbox;

	hbox = gtk_hbox_new(TRUE, 2);

	//GtkAllocation alloc;
	//gtk_widget_get_allocation(GTK_WIDGET(self), &alloc);
	//g_debug("allocation width for the transport widget %i", alloc.width);
	priv->hbox = hbox;

	priv->play_button = play_button_new();

	gtk_box_pack_start (GTK_BOX (priv->hbox), priv->play_button, FALSE, FALSE, 0);	
	
	g_signal_connect(G_OBJECT(twin_item), "property-changed", G_CALLBACK(transport_widget_property_update), self);
	                 
	gtk_container_add (GTK_CONTAINER (self), priv->hbox);

  gtk_widget_show_all (priv->hbox);
}

static void
transport_widget_dispose (GObject *object)
{
	G_OBJECT_CLASS (transport_widget_parent_class)->dispose (object);
}

static void
transport_widget_finalize (GObject *object)
{
	G_OBJECT_CLASS (transport_widget_parent_class)->finalize (object);
}

static gboolean 
transport_widget_expose_event(GtkWidget* widget, GdkEventExpose* event)
{
	//TransportWidgetPrivate * priv = TRANSPORT_WIDGET_GET_PRIVATE(widget);	
	//gtk_container_propagate_expose(GTK_CONTAINER(widget),priv->play_button, event);
	return TRUE;
}
	

/* keyevents */
static gboolean
transport_widget_button_press_event (GtkWidget *menuitem, 
                                  	GdkEventButton *event)
{
	g_debug("TransportWidget::menu_press_event");
	if(IS_TRANSPORT_WIDGET(menuitem) == FALSE){
		return FALSE;
	}

	//TransportWidgetPrivate * priv = TRANSPORT_WIDGET_GET_PRIVATE(TRANSPORT_WIDGET(menuitem));

 	//GValue value = {0};
  //g_value_init(&value, G_TYPE_BOOLEAN);
	//g_debug("TransportWidget::menu_press_event - going to send value %i", state);
	//g_value_set_boolean(&value, state);	
	//dbusmenu_menuitem_handle_event (twin_item, "Transport state change", &value, 0);
	
	return TRUE;
}

                              
static gboolean
transport_widget_button_release_event (GtkWidget *menuitem, 
                                  GdkEventButton *event)
{
	g_debug("TransportWidget::menu_release_event");
	if(IS_TRANSPORT_WIDGET(menuitem) == FALSE){
		return FALSE;
	}
	
	return TRUE;
}

/**
* transport_widget_update_state()
* Callback for updates from the other side of dbus
**/
static void 
transport_widget_property_update(DbusmenuMenuitem* item, gchar* property, 
                                 GValue* value, gpointer userdata)
{
	g_debug("transport_widget_update_state - with property  %s", property);  
	//int update_value = g_value_get_int(value);
	//g_debug("transport_widget_update_state - with value  %i", update_value);  

	//TransportWidget* bar = (TransportWidget*)userdata;
	//TransportWidgetPrivate *priv = TRANSPORT_WIDGET_GET_PRIVATE(bar);
	
}

 /**
 * transport_new:
 * @returns: a new #TransportWidget.
 **/
GtkWidget* 
transport_widget_new(DbusmenuMenuitem *item)
{
	twin_item = item;
	return g_object_new(TRANSPORT_WIDGET_TYPE, NULL);
}

