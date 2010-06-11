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
#ifndef __TRANSPORT_BAR_H__
#define __TRANSPORT_BAR_H__

#include <gtk/gtkmenuitem.h>
#include <libdbusmenu-gtk/menu.h>

G_BEGIN_DECLS

#define TRANSPORT_BAR_TYPE            (transport_bar_get_type ())
#define TRANSPORT_BAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TRANSPORT_BAR_TYPE, TransportBar))
#define TRANSPORT_BAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TRANSPORT_BAR_TYPE, TransportBarClass))
#define IS_TRANSPORT_BAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TRANSPORT_BAR_TYPE))
#define IS_TRANSPORT_BAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TRANSPORT_BAR_TYPE))
#define TRANSPORT_BAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TRANSPORT_BAR_TYPE, TransportBarClass))

typedef struct _TransportBar      TransportBar;
typedef struct _TransportBarClass TransportBarClass;

struct _TransportBarClass {
	  GtkMenuItemClass parent_class;
};

struct _TransportBar {
	  GtkMenuItem parent;
};

GType transport_bar_get_type (void);
GtkWidget* transport_bar_new();
void connect_with_other_half(DbusmenuMenuitem *twin_item);

G_END_DECLS

#endif

