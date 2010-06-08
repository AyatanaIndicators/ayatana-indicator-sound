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

typedef struct _TransportBarPrivate TransportBarPrivate;

struct _TransportBarPrivate
{
};

#define TRANSPORT_BAR_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TRANSPORT_BAR_TYPE, TransportBarPrivate))

/* Prototypes */
static void transport_bar_class_init (TransportBarClass *klass);
static void transport_bar_init       (TransportBar *self);
static void transport_bar_dispose    (GObject *object);
static void transport_bar_finalize   (GObject *object);
//static void handle_event (DbusmenuMenuitem * mi, const gchar * name, const GValue * value, guint timestamp);
G_DEFINE_TYPE (TransportBar, transport_bar, DBUSMENU_TYPE_MENUITEM);

static void
transport_bar_class_init (TransportBarClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (TransportBarPrivate));

	object_class->dispose = transport_bar_dispose;
	object_class->finalize = transport_bar_finalize;

  //DbusmenuMenuitemClass * mclass = DBUSMENU_MENUITEM_CLASS(klass);
  //mclass->handle_event = handle_event;
	return;
}

static void
transport_bar_init (TransportBar *self)
{
	g_debug("Building new Transport Item");
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


//static void
//handle_event (DbusmenuMenuitem * mi, const gchar * name, const GValue * value, guint timestamp)
//{
//	g_debug("TransportBar -> handle event caught!");
//}



TransportBar*
transport_bar_new()
{
	TransportBar *self = g_object_new(TRANSPORT_BAR_TYPE, NULL);
  //dbusmenu_menuitem_property_set(DBUSMENU_MENUITEM(self), DBUSMENU_MENUITEM_PROP_TYPE, DBUSMENU_TRANSPORT_MENUITEM_TYPE);
	return self;
}
