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
	GtkAllocation alloc;

	alloc.width = 200;
	alloc.height = 600;
	alloc.x = 100;
	alloc.y = 100;
	
	gtk_widget_set_allocation(GTK_WIDGET(button), 
	                          &alloc);

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
	double x, y;
	double radius;
	int i;
	
	x = button->allocation.x + button->allocation.width / 2;
	y = button->allocation.y + button->allocation.height / 2;
	radius = MIN (button->allocation.width / 2,
		      button->allocation.height / 2) - 5;

	/* button back */
	cairo_arc (cr, x, y, radius, 0, 2 * M_PI);
	cairo_set_source_rgb (cr, 1, 1, 1);
	cairo_fill_preserve (cr);
	cairo_set_source_rgb (cr, 0, 0, 0);
	cairo_stroke (cr);

	/* button ticks */
	for (i = 0; i < 12; i++)
	{
		int inset;
	
		cairo_save (cr); /* stack-pen-size */
		
		if (i % 3 == 0)
		{
			inset = 0.2 * radius;
		}
		else
		{
			inset = 0.1 * radius;
			cairo_set_line_width (cr, 0.5 *
					cairo_get_line_width (cr));
		}
		
		cairo_move_to (cr,
				x + (radius - inset) * cos (i * M_PI / 6),
				y + (radius - inset) * sin (i * M_PI / 6));
		cairo_line_to (cr,
				x + radius * cos (i * M_PI / 6),
				y + radius * sin (i * M_PI / 6));
		cairo_stroke (cr);
		cairo_restore (cr); /* stack-pen-size */
	}
}

/**
* play_button_new:
* @returns: a new #PlayButton.
**/
GtkWidget* 
play_button_new()
{
	return g_object_new(PLAY_BUTTON_TYPE, NULL);
}

