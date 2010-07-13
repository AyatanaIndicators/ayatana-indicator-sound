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
#ifndef __SCRUB_WIDGET_H__
#define __SCRUB_WIDGET_H__

#include <glib.h>
#include <glib-object.h>
#include <libdbusmenu-gtk/menu.h>

G_BEGIN_DECLS

#define SCRUB_WIDGET_TYPE            (scrub_widget_get_type ())
#define SCRUB_WIDGET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SCRUB_WIDGET_TYPE, ScrubWidget))
#define SCRUB_WIDGET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SCRUB_WIDGET_TYPE, ScrubWidgetClass))
#define IS_SCRUB_WIDGET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SCRUB_WIDGET_TYPE))
#define IS_SCRUB_WIDGET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SCRUB_WIDGET_TYPE))
#define SCRUB_WIDGET_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), SCRUB_WIDGET_TYPE, ScrubWidgetClass))

typedef struct _ScrubWidget	ScrubWidget;
typedef struct _ScrubWidgetClass	ScrubWidgetClass;

struct _ScrubWidgetClass {
  GObjectClass parent_class;
};

struct _ScrubWidget {
	GObject parent;
};

GType scrub_widget_get_type (void) G_GNUC_CONST;
GtkWidget* scrub_widget_new(DbusmenuMenuitem* twin_item);

G_END_DECLS

#endif

