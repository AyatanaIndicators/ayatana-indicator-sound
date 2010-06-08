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
	GtkWidget* hBox;
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
G_DEFINE_TYPE (TransportBar, transport_bar, DBUSMENU_TYPE_MENUITEM);

static void
transport_bar_class_init (TransportBarClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (TransportBarPrivate));

	object_class->dispose = transport_bar_dispose;
	object_class->finalize = transport_bar_finalize;

	return;
}

static void
transport_bar_init (TransportBar *self)
{
	g_debug("Building new Transport Item");
	hBox = gtk_hbox_new(TRUE, 2));
	previous_button = gtk_button_new_with_label("Previous"));
  next_button = gtk_button_new_with_label("Next"));
	play_button =	gtk_button_new_with_label("Play"));
	gtk_container_add((GtkContainer*) hBox, previous_button); 
	gtk_container_add((GtkContainer*) hBox, next_button); 
	gtk_container_add((GtkContainer*) hBox, play_button); 
	return;
}

static void
transport_bar_dispose (GObject *object)
{
	G_OBJECT_CLASS (transport_bar_parent_class)->dispose (object);
	return;
}

static void
transport_bar_finalize (GObject *object)
{
	G_OBJECT_CLASS (transport_bar_parent_class)->finalize (object);
}

TransportBar*
transport_bar_new()
{
	TransportBar *self = g_object_new(TRANSPORT_BAR_TYPE, NULL);
	return self;
}
