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

#include <math.h>
#include "play-button.h"


typedef struct _PlayButtonPrivate PlayButtonPrivate;

struct _PlayButtonPrivate
{
	
};

#define PLAY_BUTTON_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), PLAY_BUTTON_TYPE, PlayButtonPrivate))

/* Gobject boiler plate */
static void play_button_class_init (PlayButtonClass *klass);
static void play_button_init       (PlayButton *self);
static void play_button_dispose    (GObject *object);
static void play_button_finalize   (GObject *object);

static gboolean play_button_expose (GtkWidget *button, GdkEventExpose *event);
static void draw (GtkWidget* button, cairo_t *cr);
                                          
G_DEFINE_TYPE (PlayButton, play_button, GTK_TYPE_DRAWING_AREA);


static void
play_button_class_init (PlayButtonClass *klass)
{
	
	GObjectClass	*gobject_class = G_OBJECT_CLASS (klass);
 	GtkWidgetClass* widget_class = GTK_WIDGET_CLASS (klass);

	g_type_class_add_private (klass, sizeof (PlayButtonPrivate));

 	widget_class->expose_event = play_button_expose;

	gobject_class->dispose = play_button_dispose;
	gobject_class->finalize = play_button_finalize;
}

static void
play_button_init (PlayButton *self)
{
	g_debug("PlayButton::play_button_init");	
	gtk_widget_set_size_request(GTK_WIDGET(self), 200, 80); 
}

static void
play_button_dispose (GObject *object)
{
	G_OBJECT_CLASS (play_button_parent_class)->dispose (object);
}

static void
play_button_finalize (GObject *object)
{
	G_OBJECT_CLASS (play_button_parent_class)->finalize (object);
}

static gboolean
play_button_expose (GtkWidget *button, GdkEventExpose *event)
{
	cairo_t *cr;
  cr = gdk_cairo_create (button->window);

	g_debug("PlayButton::Draw - width = %i", button->allocation.width);
	g_debug("PlayButton::Draw - event->area.width = %i", event->area.width);
	g_debug("PlayButton::Draw - event->area.x = %i", event->area.x);

	cairo_rectangle (cr,
                   event->area.x, event->area.y,
                   event->area.width, event->area.height);

	cairo_clip(cr);
	draw (button, cr);
  cairo_destroy (cr);
	return FALSE;
}


static void
draw (GtkWidget* button, cairo_t *cr)
{
	int rect_width = 150;
	int rect_height = 30;

	double x= button->allocation.width/2 - rect_width/2;
	double y= button->allocation.height/2 -rect_height/2;
	double radius=60;
	
	
	cairo_move_to(cr, x+radius, y);
	cairo_line_to(cr, x+rect_width-radius, y);
	cairo_curve_to(cr, x+rect_width, y, x+rect_width, y, x+rect_width, y+radius); 

	cairo_line_to(cr, x + rect_width, y + rect_height - radius);              
  cairo_curve_to(cr, x + rect_width, y + rect_height, x + rect_width,
                 y + rect_height, x + rect_width - radius, y + rect_height);

	cairo_line_to(cr, x + radius, y + rect_height);
	cairo_curve_to(cr, x, y + rect_height, x, y+rect_height, x, y+rect_height-radius);
	cairo_line_to(cr, x, y + radius);	
	cairo_curve_to(cr, x, y, x, y, x + radius, y);

	cairo_arc(cr, x+(rect_width/2), y+(rect_height/2), radius/2, 0, 2 * M_PI);

	cairo_set_source_rgb (cr, 1, 1, 1);
	cairo_fill_preserve (cr);

  //      cr.fill()
  //      cr.stroke()	
	//cairo_set_source_rgb (cr, 1, 1, 1);
	//cairo_stroke (cr);
	 //cairo_surface_write_to_png(cairo_get_target (cr), "/tmp/foobar.png");	
}



/**
* play_button_new:
* @returns: a new #PlayButton.
**/
GtkWidget* 
play_button_new()
{
	GtkWidget* widget =	g_object_new(PLAY_BUTTON_TYPE, NULL);
	gtk_widget_set_app_paintable (widget, TRUE);	
	return widget;
}

