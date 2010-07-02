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
	GdkColor 	background_colour_fg;
	GdkColor 	background_colour_bg_dark;
	GdkColor 	background_colour_bg_light;
	GdkColor 	foreground_colour_fg;
	GdkColor 	foreground_colour_bg;
};

#define PLAY_BUTTON_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), PLAY_BUTTON_TYPE, PlayButtonPrivate))

/* Gobject boiler plate */
static void play_button_class_init (PlayButtonClass *klass);
static void play_button_init       (PlayButton *self);
static void play_button_dispose    (GObject *object);
static void play_button_finalize   (GObject *object);

static gboolean play_button_expose (GtkWidget *button, GdkEventExpose *event);
static void draw (GtkWidget* button, cairo_t *cr);
static void play_button_draw_background(GtkWidget* button, cairo_t* cr, double x, double y, double width, double height, double p_radius);
static void play_button_draw_background_shadow_2(GtkWidget* button, cairo_t* cr, double x, double y, double rect_width, double rect_height,  double p_radius);
static void play_button_draw_background_shadow_1(GtkWidget* button, cairo_t* cr, double x, double y, double rect_width, double rect_height, double p_radius);


//static void play_button_draw_play_symbol(cairo_t* cr, double x, double y);
static void play_button_draw_pause_symbol(cairo_t* cr, double x, double y);


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


void
play_button_set_style(GtkWidget* button, GtkStyle* style)
{
	PlayButtonPrivate* priv = PLAY_BUTTON_GET_PRIVATE(button);
	priv->background_colour_fg = style->fg[GTK_STATE_NORMAL];
	priv->background_colour_bg_dark = style->bg[GTK_STATE_NORMAL];
	priv->background_colour_bg_light = style->base[GTK_STATE_NORMAL];
	priv->foreground_colour_fg = style->fg[GTK_STATE_PRELIGHT];
	priv->foreground_colour_bg = style->bg[GTK_STATE_NORMAL];
}

static void
draw (GtkWidget* button, cairo_t *cr)
{

	//PlayButtonPrivate* priv = PLAY_BUTTON_GET_PRIVATE(button);
	double rect_width = 115;
	double rect_height = 28;
	double p_radius = 21;
	double y = 15;
	double x = 22;
	//double radius=35;
	//double x= button->allocation.width/2 - rect_width/2;
	//double y= button->allocation.height/2 -rect_height/2;

	// Draw the outside drop shadow background
	play_button_draw_background_shadow_1(button, cr, x, y, rect_width, rect_height, p_radius);
	
	// Draw the inside drop shadow background
	gint offset = 1.5;
	play_button_draw_background_shadow_2(button, cr, x+ offset/2, y + offset/2, rect_width-offset, rect_height-offset, p_radius-offset/2);

	offset = 3;
	// Draw the inside actual background
	play_button_draw_background(button, cr, x+offset/2, y + offset/2, rect_width-offset, rect_height-offset, p_radius-offset/2);

	play_button_draw_pause_symbol(cr, rect_width/2 + rect_height/10 + x, rect_height/5 +y );
	cairo_surface_write_to_png(cairo_get_target (cr), "/tmp/foobar.png");	
}

static void 
play_button_draw_pause_symbol(cairo_t* cr, double x, double y)
{
	cairo_set_line_width (cr, 6.0);
	cairo_set_line_cap  (cr, CAIRO_LINE_CAP_ROUND);
	cairo_move_to (cr, x, y);
	cairo_rel_line_to (cr, 0, 16);
	//cairo_set_source_rgb(cr, 94/255.0, 93/255.0, 90/255.0);

	cairo_pattern_t *pat;
	pat = cairo_pattern_create_linear (x, y,  x, y+16);
	cairo_pattern_add_color_stop_rgb(pat, 0, 227/255.0, 222/255.0, 214/255.0);
	cairo_pattern_add_color_stop_rgb(pat, .1, 94/255.0, 93/255.0, 90/255.0);
	cairo_set_source (cr, pat);	
	cairo_stroke(cr);
	cairo_close_path(cr);

	cairo_set_line_width (cr, 5.0);
	cairo_set_line_cap  (cr, CAIRO_LINE_CAP_ROUND);
	cairo_move_to (cr, x+1, y+1);
	cairo_rel_line_to (cr, 0, 15);

	pat = cairo_pattern_create_linear (x+1, y+1,  x+1, y+16);
	cairo_pattern_add_color_stop_rgb(pat, .7, 252/255.0, 252/255.0, 251/255.0);
	cairo_pattern_add_color_stop_rgb(pat, .9, 207/255.0, 201/255.0, 190/255.0);
	cairo_set_source (cr, pat);	
	cairo_stroke(cr);
	cairo_close_path(cr);
	
	cairo_pattern_destroy (pat);
}

static void
play_button_draw_background(GtkWidget* button, cairo_t* cr, double x, double y, double rect_width, double rect_height, double p_radius)
{
	double radius=rect_height/2;
	cairo_set_line_width (cr, rect_height);
	cairo_set_line_cap  (cr, CAIRO_LINE_CAP_ROUND);
	cairo_move_to(cr, x+radius, y+radius);
	cairo_line_to(cr, x+rect_width, y+radius);

	cairo_pattern_t *pat;
	pat = cairo_pattern_create_linear (x, y,  x, y+rect_height);
	cairo_pattern_add_color_stop_rgb(pat, .7, 227/255.0, 222/255.0, 214/255.0);
	cairo_pattern_add_color_stop_rgb(pat, .9, 183/255.0, 178/255.0, 172/255.0);
	cairo_set_source (cr, pat);
	cairo_stroke (cr);

	cairo_close_path(cr);
	cairo_arc(cr, rect_width/2 + radius/2 + x, rect_height/2 +y, p_radius, 0, 2*M_PI);
	cairo_set_source (cr, pat);	
	cairo_fill(cr);

	cairo_close_path(cr);
	cairo_arc(cr, rect_width/2 + radius/2 + x, rect_height/2 +y, p_radius, 0, 2*M_PI);
	cairo_set_source_rgb(cr, 94/255.0, 93/255.0, 90/255.0);
	cairo_set_line_width (cr, 2);	
	cairo_stroke(cr);
	cairo_close_path(cr);	
	cairo_pattern_destroy (pat);
}

static void
play_button_draw_background_shadow_1(GtkWidget* button, cairo_t* cr, double x, double y, double rect_width, double rect_height, double p_radius)
{
	double radius=rect_height/2;

	cairo_set_line_width (cr, rect_height);
	cairo_set_line_cap  (cr, CAIRO_LINE_CAP_ROUND);
	cairo_move_to(cr, x+radius, y+radius);
	cairo_line_to(cr, x+rect_width, y+radius);

	cairo_pattern_t *pat;
	pat = cairo_pattern_create_linear (0, 0,  0, rect_height);
	cairo_pattern_add_color_stop_rgb(pat, .2, 36/255.0, 35/255.0, 33/255.0);
	cairo_pattern_add_color_stop_rgb(pat, .7, 123/255.0, 123/255.0, 120/255.0);
	cairo_set_source (cr, pat);
	cairo_stroke (cr);

	cairo_close_path(cr);
	cairo_arc(cr, rect_width/2 + radius/2 + x, rect_height/2 +y, p_radius, 0, 2*M_PI);
	pat = cairo_pattern_create_linear (0, 0,  0, rect_height+(p_radius-rect_height/2));
	cairo_pattern_add_color_stop_rgb(pat, .2, 36/255.0, 35/255.0, 33/255.0);
	cairo_pattern_add_color_stop_rgb(pat, .7, 123/255.0, 123/255.0, 120/255.0);
	cairo_set_source (cr, pat);	
	cairo_fill(cr);
	cairo_close_path(cr);
	cairo_pattern_destroy (pat);
	
}

static void
play_button_draw_background_shadow_2(GtkWidget* button, cairo_t* cr, double x, double y, double rect_width, double rect_height, double p_radius)
{
	double radius=rect_height/2;
	
	cairo_set_line_width (cr, rect_height);
	cairo_set_line_cap  (cr, CAIRO_LINE_CAP_ROUND);
	cairo_move_to(cr, x+radius, y+radius);
	cairo_line_to(cr, x+rect_width, y+radius);

	cairo_pattern_t *pat;
	pat = cairo_pattern_create_linear (0, 0,  0, rect_height);
	cairo_pattern_add_color_stop_rgb(pat, .2, 61/255.0, 60/255.0, 57/255.0);
	cairo_pattern_add_color_stop_rgb(pat, .7, 94/255.0, 93/255.0, 90/255.0);
	cairo_set_source (cr, pat);
	cairo_stroke (cr);
	
	cairo_close_path(cr);
	cairo_arc(cr, rect_width/2 + radius/2 + x, rect_height/2 +y, p_radius, 0, 2*M_PI);
	pat = cairo_pattern_create_linear (0, 0,  0, rect_height+(p_radius-rect_height/2));
	cairo_pattern_add_color_stop_rgb(pat, .2, 61/255.0, 60/255.0, 57/255.0);
	cairo_pattern_add_color_stop_rgb(pat, .7, 94/255.0, 93/255.0, 90/255.0);
	
	cairo_set_source (cr, pat);	
	cairo_fill(cr);
	cairo_close_path(cr);	
	cairo_pattern_destroy (pat);
	
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

