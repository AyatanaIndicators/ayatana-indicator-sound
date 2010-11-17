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
#ifndef __TRANSPORT_WIDGET_H__
#define __TRANSPORT_WIDGET_H__

#include <gtk/gtk.h>
#include <gtk/gtkmenuitem.h>
#include <libdbusmenu-gtk/menuitem.h>

G_BEGIN_DECLS

#define TRANSPORT_WIDGET_TYPE            (transport_widget_get_type ())
#define TRANSPORT_WIDGET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TRANSPORT_WIDGET_TYPE, TransportWidget))
#define TRANSPORT_WIDGET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TRANSPORT_WIDGET_TYPE, TransportWidgetClass))
#define IS_TRANSPORT_WIDGET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TRANSPORT_WIDGET_TYPE))
#define IS_TRANSPORT_WIDGET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TRANSPORT_WIDGET_TYPE))
#define TRANSPORT_WIDGET_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TRANSPORT_WIDGET_TYPE, TransportWidgetClass))

typedef struct _TransportWidget      TransportWidget;
typedef struct _TransportWidgetClass TransportWidgetClass;

typedef enum {
	TRANSPORT_PREVIOUS,
	TRANSPORT_PLAY_PAUSE,
	TRANSPORT_NEXT,
	TRANSPORT_NADA
}TransportWidgetEvent;

typedef enum {
	PLAY,
	PAUSE	
}TransportWidgetState;

struct _TransportWidgetClass {
	  GtkMenuItemClass parent_class;
};

struct _TransportWidget {
	  GtkMenuItem parent;
};

typedef struct
{
	double r;
	double g;
	double b;
} CairoColorRGB;


void _color_shade (const CairoColorRGB *a, float k, CairoColorRGB *b);
GType transport_widget_get_type (void);
GtkWidget* transport_widget_new ( DbusmenuMenuitem *item );
void transport_widget_react_to_key_press_event ( TransportWidget* widget,
                                                 TransportWidgetEvent transport_event );
void transport_widget_react_to_key_release_event ( TransportWidget* widget,
                                                   TransportWidgetEvent transport_event );
G_END_DECLS

#endif

