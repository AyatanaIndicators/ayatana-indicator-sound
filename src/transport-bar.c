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
#include "transport-bar.h"
#include "common-defs.h"
#include <gtk/gtk.h>

typedef struct _TransportBarPrivate TransportBarPrivate;

struct _TransportBarPrivate
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

#define TRANSPORT_BAR_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TRANSPORT_BAR_TYPE, TransportBarPrivate))

/* Prototypes */
static void transport_bar_class_init (TransportBarClass *klass);
static void transport_bar_init       (TransportBar *self);
static void transport_bar_dispose    (GObject *object);
static void transport_bar_finalize   (GObject *object);


static gboolean transport_bar_button_press_event (GtkWidget             *menuitem,
                                                  GdkEventButton        *event);
static gboolean transport_bar_button_release_event (GtkWidget             *menuitem,
                                                    GdkEventButton        *event);
static gboolean transport_bar_play_button_trigger (GtkWidget* widget, 
																											 GdkEventButton *event,
                                      								 gpointer        user_data);


G_DEFINE_TYPE (TransportBar, transport_bar, GTK_TYPE_MENU_ITEM);

static void
transport_bar_class_init (TransportBarClass *klass)
{
	GObjectClass 			*gobject_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass    *widget_class = GTK_WIDGET_CLASS (klass);

  widget_class->button_press_event = transport_bar_button_press_event;
  widget_class->button_release_event = transport_bar_button_release_event;
	
	g_type_class_add_private (klass, sizeof (TransportBarPrivate));

	gobject_class->dispose = transport_bar_dispose;
	gobject_class->finalize = transport_bar_finalize;

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
transport_bar_init (TransportBar *self)
{
	g_debug("TransportBar::transport_bar_init");

	TransportBarPrivate * priv = TRANSPORT_BAR_GET_PRIVATE(self);
	GtkWidget *hbox;

	hbox = gtk_hbox_new(TRUE, 2);
	priv->previous_button = gtk_button_new_with_label("<<");
  priv->next_button = gtk_button_new_with_label(">>");
	priv->play_button =	gtk_button_new_with_label(">");

	gtk_box_pack_start (GTK_BOX (hbox), priv->previous_button, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), priv->play_button, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), priv->next_button, FALSE, FALSE, 0);

	g_signal_connect(priv->play_button, "button-press-event", G_CALLBACK(transport_bar_play_button_trigger), NULL);

	
  priv->hbox = hbox;

  gtk_widget_show_all (priv->hbox);
  gtk_container_add (GTK_CONTAINER (self), hbox);

	
}

static void
transport_bar_dispose (GObject *object)
{
	G_OBJECT_CLASS (transport_bar_parent_class)->dispose (object);
}

static void
transport_bar_finalize (GObject *object)
{
	G_OBJECT_CLASS (transport_bar_parent_class)->finalize (object);
}

/* keyevents */
static gboolean
transport_bar_button_press_event (GtkWidget *menuitem, 
                                  GdkEventButton *event)
{
	g_debug("TransportBar::menu_press_event");
	return FALSE;
}

static gboolean
transport_bar_button_release_event (GtkWidget *menuitem, 
                                  GdkEventButton *event)
{
	g_debug("TransportBar::menu_release_event");
	return FALSE;
}

/* Individual keyevents on the buttons */
static gboolean
transport_bar_play_button_trigger(GtkWidget* widget,
                                      GdkEventButton *event,
                                      gpointer        user_data)
{
	g_debug("TransportBar::PLAY button_press_event");	
	return FALSE;
}

 /**
 * transport_new:
 * @returns: a new #TransportBar.
 **/
GtkWidget* 
transport_bar_new()
{
	return g_object_new(TRANSPORT_BAR_TYPE, NULL);
}

