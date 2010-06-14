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

// TODO: think about leakage: ted !

static DbusmenuMenuitem* twin_item;

typedef struct _TransportWidgetPrivate TransportWidgetPrivate;

struct _TransportWidgetPrivate
{
	GtkWidget* hbox;
	GtkWidget* previous_button;
	GtkWidget* play_button;
	GtkWidget* next_button;	
};

enum {
  PLAY,
	PAUSE,
  NEXT,
	PREVIOUS,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

#define TRANSPORT_WIDGET_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TRANSPORT_WIDGET_TYPE, TransportWidgetPrivate))

/* Gobject boiler plate */
static void transport_widget_class_init (TransportWidgetClass *klass);
static void transport_widget_init       (TransportWidget *self);
static void transport_widget_dispose    (GObject *object);
static void transport_widget_finalize   (GObject *object);

/* UI and dbus callbacks */
static gboolean transport_widget_button_press_event (GtkWidget             *menuitem,
                                                  GdkEventButton        *event);
static gboolean transport_widget_button_release_event (GtkWidget             *menuitem,
                                                    GdkEventButton        *event);
static gboolean transport_widget_play_button_trigger (GtkWidget 		 	*widget, 
																									GdkEventButton 	*event,
                                      						gpointer        user_data);
static void transport_widget_update_state(DbusmenuMenuitem* item,
                                       gchar * property, 
                                       GValue * value,
                                       gpointer userdata);
// utility methods
static gchar* transport_widget_determine_play_label(gchar* state);

G_DEFINE_TYPE (TransportWidget, transport_widget, GTK_TYPE_MENU_ITEM);



static void
transport_widget_class_init (TransportWidgetClass *klass)
{
	GObjectClass 			*gobject_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass    *widget_class = GTK_WIDGET_CLASS (klass);

  widget_class->button_press_event = transport_widget_button_press_event;
  widget_class->button_release_event = transport_widget_button_release_event;
	
	g_type_class_add_private (klass, sizeof (TransportWidgetPrivate));

	gobject_class->dispose = transport_widget_dispose;
	gobject_class->finalize = transport_widget_finalize;

  signals[PLAY] = g_signal_new ("play",
                               G_OBJECT_CLASS_TYPE (gobject_class),
                               G_SIGNAL_RUN_FIRST,
                               0,
                               NULL, NULL,
                               g_cclosure_marshal_VOID__VOID,
                               G_TYPE_NONE, 0);

  signals[PAUSE] = g_signal_new ("pause",
                               G_OBJECT_CLASS_TYPE (gobject_class),
                               G_SIGNAL_RUN_FIRST,
                               0,
                               NULL, NULL,
                               g_cclosure_marshal_VOID__VOID,
                               G_TYPE_NONE, 0);


	signals[NEXT] = g_signal_new ("next",
                               G_OBJECT_CLASS_TYPE (gobject_class),
                               G_SIGNAL_RUN_FIRST,
                               0,
                               NULL, NULL,
                               g_cclosure_marshal_VOID__VOID,
                               G_TYPE_NONE, 0);

  signals[PREVIOUS] = g_signal_new ("previous",
                               G_OBJECT_CLASS_TYPE (gobject_class),
                               G_SIGNAL_RUN_FIRST,
                               0,
                               NULL, NULL,
                               g_cclosure_marshal_VOID__VOID,
                               G_TYPE_NONE, 0);	
}

static void
transport_widget_init (TransportWidget *self)
{
	g_debug("TransportWidget::transport_widget_init");

	TransportWidgetPrivate * priv = TRANSPORT_WIDGET_GET_PRIVATE(self);
	GtkWidget *hbox;

	hbox = gtk_hbox_new(TRUE, 2);
	priv->previous_button = gtk_button_new_with_label("<<");
  priv->next_button = gtk_button_new_with_label(">>");
	priv->play_button =	gtk_button_new_with_label(">");

	gtk_box_pack_start (GTK_BOX (hbox), priv->previous_button, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), priv->play_button, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), priv->next_button, FALSE, FALSE, 0);

	g_signal_connect(priv->play_button, "button-press-event", G_CALLBACK(transport_widget_play_button_trigger), NULL);
  priv->hbox = hbox;
	
	g_signal_connect(G_OBJECT(twin_item), "property-changed", G_CALLBACK(transport_widget_update_state), self);

  gtk_widget_show_all (priv->hbox);
  gtk_container_add (GTK_CONTAINER (self), hbox);
	
}

static void
transport_widget_dispose (GObject *object)
{
	//if(IS_TRANSPORT_BAR(object) == TRUE){ 
	//	TransportWidgetPrivate * priv = TRANSPORT_BAR_GET_PRIVATE(TRANSPORT_BAR(object));
	//	g_object_unref(priv->previous_button);
	//}
	G_OBJECT_CLASS (transport_widget_parent_class)->dispose (object);
}

static void
transport_widget_finalize (GObject *object)
{
	G_OBJECT_CLASS (transport_widget_parent_class)->finalize (object);
}

/* keyevents */
static gboolean
transport_widget_button_press_event (GtkWidget *menuitem, 
                                  GdkEventButton *event)
{
	g_debug("TransportWidget::menu_press_event");
	return FALSE;
}

static gboolean
transport_widget_button_release_event (GtkWidget *menuitem, 
                                  GdkEventButton *event)
{
	g_debug("TransportWidget::menu_release_event");
	return FALSE;
}

/* Individual keyevents on the buttons */
static gboolean
transport_widget_play_button_trigger(GtkWidget* widget,
                                  GdkEventButton *event,
                                  gpointer        user_data)
{
	g_debug("TransportWidget::PLAY button_press_event");	
	return FALSE;
}

/**
* transport_widget_update_state()
* Callback for updates from the other side of dbus
**/
static void transport_widget_update_state(DbusmenuMenuitem* item, gchar* property, 
                                       GValue* value, gpointer userdata)
{
	g_debug("transport_widget_update_state - with property  %s", property);  
	gchar* input = g_strdup(g_value_get_string(value));
	g_debug("transport_widget_update_state - with value  %s", input);  

	TransportWidget* bar = (TransportWidget*)userdata;
	TransportWidgetPrivate *priv = TRANSPORT_WIDGET_GET_PRIVATE(bar);
	
	gtk_button_set_label(GTK_BUTTON(priv->play_button), g_strdup(transport_widget_determine_play_label(property)));
}

// will be needed for image swapping
static gchar* transport_widget_determine_play_label(gchar* state)
{
	gchar* label = ">";
	if(g_strcmp0(state, "pause") == 0){	
		label = "||";
	}
	else if(g_strcmp0(state, "play") == 0){
		label = ">";
	}
	return label;
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

