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

#define TRANSPORT_BAR_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TRANSPORT_BAR_TYPE, TransportBarPrivate))

/* Prototypes */
static void transport_bar_class_init (TransportBarClass *klass);
static void transport_bar_init       (TransportBar *self);
static void transport_bar_dispose    (GObject *object);
static void transport_bar_finalize   (GObject *object);

G_DEFINE_TYPE (TransportBar, transport_bar, GTK_TYPE_MENU_ITEM);

static void
transport_bar_class_init (TransportBarClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (TransportBarPrivate));

	object_class->dispose = transport_bar_dispose;
	object_class->finalize = transport_bar_finalize;
}

static void
transport_bar_init (TransportBar *self)
{
	g_debug("TransportBar::transport_bar_init");

	TransportBarPrivate * priv = TRANSPORT_BAR_GET_PRIVATE(self);
	GtkWidget *hbox;

	hbox = gtk_hbox_new(TRUE, 2);
	priv->previous_button = gtk_button_new_with_label("Previous");
  priv->next_button = gtk_button_new_with_label("Next");
	priv->play_button =	gtk_button_new_with_label("Play");

	gtk_box_pack_start (GTK_BOX (hbox), priv->previous_button, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), priv->play_button, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), priv->next_button, FALSE, FALSE, 0);
	
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

/**
 * transport_new:
 * @returns: a new #TransportBar.
 **/
GtkWidget* 
transport_bar_new()
{
	return g_object_new(TRANSPORT_BAR_TYPE, NULL);
}

