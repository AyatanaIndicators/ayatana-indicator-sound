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

Uses code from ctk
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
static void play_button_draw_previous_symbol(cairo_t* cr, double x, double y);


G_DEFINE_TYPE (PlayButton, play_button, GTK_TYPE_DRAWING_AREA);

/// internal helper functions //////////////////////////////////////////////////

static double
_align (double val)
{
  double fract = val - (int) val;

  if (fract != 0.5f)
    return (double) ((int) val + 0.5f);
  else
    return val;
}

static inline void
_blurinner (guchar* pixel,
	    gint*   zR,
	    gint*   zG,
	    gint*   zB,
	    gint*   zA,
	    gint    alpha,
	    gint    aprec,
	    gint    zprec)
{
	gint R;
	gint G;
	gint B;
	guchar A;

	R = *pixel;
	G = *(pixel + 1);
	B = *(pixel + 2);
	A = *(pixel + 3);

	*zR += (alpha * ((R << zprec) - *zR)) >> aprec;
	*zG += (alpha * ((G << zprec) - *zG)) >> aprec;
	*zB += (alpha * ((B << zprec) - *zB)) >> aprec;
	*zA += (alpha * ((A << zprec) - *zA)) >> aprec;

	*pixel       = *zR >> zprec;
	*(pixel + 1) = *zG >> zprec;
	*(pixel + 2) = *zB >> zprec;
	*(pixel + 3) = *zA >> zprec;
}

static inline void
_blurrow (guchar* pixels,
	  gint    width,
	  gint    height,
	  gint    channels,
	  gint    line,
	  gint    alpha,
	  gint    aprec,
	  gint    zprec)
{
	gint    zR;
	gint    zG;
	gint    zB;
	gint    zA;
	gint    index;
	guchar* scanline;

	scanline = &(pixels[line * width * channels]);

	zR = *scanline << zprec;
	zG = *(scanline + 1) << zprec;
	zB = *(scanline + 2) << zprec;
	zA = *(scanline + 3) << zprec;

	for (index = 0; index < width; index ++)
		_blurinner (&scanline[index * channels],
			    &zR,
			    &zG,
			    &zB,
			    &zA,
			    alpha,
			    aprec,
			    zprec);

	for (index = width - 2; index >= 0; index--)
		_blurinner (&scanline[index * channels],
			    &zR,
			    &zG,
			    &zB,
			    &zA,
			    alpha,
			    aprec,
			    zprec);
}

static inline void
_blurcol (guchar* pixels,
	  gint    width,
	  gint    height,
	  gint    channels,
	  gint    x,
	  gint    alpha,
	  gint    aprec,
	  gint    zprec)
{
	gint zR;
	gint zG;
	gint zB;
	gint zA;
	gint index;
	guchar* ptr;

	ptr = pixels;

	ptr += x * channels;

	zR = *((guchar*) ptr    ) << zprec;
	zG = *((guchar*) ptr + 1) << zprec;
	zB = *((guchar*) ptr + 2) << zprec;
	zA = *((guchar*) ptr + 3) << zprec;

	for (index = width; index < (height - 1) * width; index += width)
		_blurinner ((guchar*) &ptr[index * channels],
			    &zR,
			    &zG,
			    &zB,
			    &zA,
			    alpha,
			    aprec,
			    zprec);

	for (index = (height - 2) * width; index >= 0; index -= width)
		_blurinner ((guchar*) &ptr[index * channels],
			    &zR,
			    &zG,
			    &zB,
			    &zA,
			    alpha,
			    aprec,
			    zprec);
}

void
_expblur (guchar* pixels,
	  gint    width,
	  gint    height,
	  gint    channels,
	  gint    radius,
	  gint    aprec,
	  gint    zprec)
{
	gint alpha;
	gint row = 0;
	gint col = 0;

	if (radius < 1)
		return;

	// calculate the alpha such that 90% of 
	// the kernel is within the radius.
	// (Kernel extends to infinity)
	alpha = (gint) ((1 << aprec) * (1.0f - expf (-2.3f / (radius + 1.f))));

	for (; row < height; row++)
		_blurrow (pixels,
			  width,
			  height,
			  channels,
			  row,
			  alpha,
			  aprec,
			  zprec);

	for(; col < width; col++)
		_blurcol (pixels,
			  width,
			  height,
			  channels,
			  col,
			  alpha,
			  aprec,
			  zprec);

	return;
}

void
_surface_blur (cairo_surface_t* surface,
               guint            radius)
{
	guchar*        pixels;
	guint          width;
	guint          height;
	cairo_format_t format;

	// before we mess with the surface execute any pending drawing
	cairo_surface_flush (surface);

	pixels = cairo_image_surface_get_data (surface);
	width  = cairo_image_surface_get_width (surface);
	height = cairo_image_surface_get_height (surface);
	format = cairo_image_surface_get_format (surface);

	switch (format)
	{
		case CAIRO_FORMAT_ARGB32:
			_expblur (pixels, width, height, 4, radius, 16, 7);
		break;

		case CAIRO_FORMAT_RGB24:
			_expblur (pixels, width, height, 3, radius, 16, 7);
		break;

		case CAIRO_FORMAT_A8:
			_expblur (pixels, width, height, 1, radius, 16, 7);
		break;

		default :
			// do nothing
		break;
	}

	// inform cairo we altered the surfaces contents
	cairo_surface_mark_dirty (surface);	
}

/// GObject functions //////////////////////////////////////////////////////////

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
draw_gradient (cairo_t* cr,
               double   x,
               double   y,
               double   w,
               double   r,
               double*  rgba_start,
               double*  rgba_end)
{
	cairo_pattern_t* pattern = NULL;

	cairo_move_to (cr, x, y);
	cairo_line_to (cr, x + w - 2.0f * r, y);
	cairo_arc (cr,
		   x + w - 2.0f * r,
		   y + r,
		   r,
		   -90.0f * G_PI / 180.0f,
		   90.0f * G_PI / 180.0f);
	cairo_line_to (cr, x, y + 2.0f * r);
	cairo_arc (cr,
		   x,
		   y + r,
		   r,
		   90.0f * G_PI / 180.0f,
		   270.0f * G_PI / 180.0f);
	cairo_close_path (cr);

	pattern = cairo_pattern_create_linear (x, y, x, y + 2.0f * r);
	cairo_pattern_add_color_stop_rgba (pattern,
	                                   0.0f,
	                                   rgba_start[0],
	                                   rgba_start[1],
	                                   rgba_start[2],
	                                   rgba_start[3]);
	cairo_pattern_add_color_stop_rgba (pattern,
	                                   1.0f,
	                                   rgba_end[0],
	                                   rgba_end[1],
	                                   rgba_end[2],
	                                   rgba_end[3]);
	cairo_set_source (cr, pattern);
	cairo_fill (cr);
	cairo_pattern_destroy (pattern);
}

static void
draw_circle (cairo_t* cr,
	     double   x,
	     double   y,
	     double   r,
	     double*  rgba_start,
	     double*  rgba_end)
{
	cairo_pattern_t* pattern = NULL;

	cairo_move_to (cr, x, y);
	cairo_arc (cr,
		   x + r,
		   y + r,
		   r,
		   0.0f * G_PI / 180.0f,
		   360.0f * G_PI / 180.0f);

	pattern = cairo_pattern_create_linear (x, y, x, y + 2.0f * r);
	cairo_pattern_add_color_stop_rgba (pattern,
	                                   0.0f,
	                                   rgba_start[0],
	                                   rgba_start[1],
	                                   rgba_start[2],
	                                   rgba_start[3]);
	cairo_pattern_add_color_stop_rgba (pattern,
	                                   1.0f,
	                                   rgba_end[0],
	                                   rgba_end[1],
	                                   rgba_end[2],
	                                   rgba_end[3]);
	cairo_set_source (cr, pattern);
	cairo_fill (cr);
	cairo_pattern_destroy (pattern);
}

static void
draw (GtkWidget* button, cairo_t *cr)
{
	//PlayButtonPrivate* priv = PLAY_BUTTON_GET_PRIVATE(button);
	double rect_width = 130;
	double y = 15;
	double x = 22;
	double inner_height   = 25.0f;
	double inner_radius   = 12.5f;
	double inner_start[]  = {229.0f / 255.0f,
			         223.0f / 255.0f,
			         215.0f / 255.0f,
			         1.0f};
	double inner_end[]    = {183.0f / 255.0f,
				 178.0f / 255.0f,
				 172.0f / 255.0f,
				 1.0f};
	double middle_height  = 27.0f;
	double middle_radius  = 13.5f;
	double middle_start[] = {61.0f / 255.0f,
				 60.0f / 255.0f,
				 57.0f / 255.0f,
				 1.0f};
	double middle_end[]   = {94.0f / 255.0f,
				 93.0f / 255.0f,
				 90.0f / 255.0f,
				 1.0f};
	double outter_height  = 29.0f;
	double outter_radius  = 14.5f;
	double outter_start[] = {36.0f / 255.0f,
				 35.0f / 255.0f,
				 33.0f / 255.0f,
				 1.0f};
	double outter_end[]   = {123.0f / 255.0f,
				 123.0f / 255.0f,
				 120.0f / 255.0f,
				 1.0f};

	double circle_radius = 19.0f;

	//double radius=35;
	//double x= button->allocation.width/2 - rect_width/2;
	//double y= button->allocation.height/2 -rect_height/2;

	// ffwd/back-background
        draw_gradient (cr,
                       x,
                       y,
                       rect_width,
                       outter_radius,
                       outter_start,
                       outter_end);
        draw_gradient (cr,
                       x,
                       y + 1,
                       rect_width - 2,
                       middle_radius,
                       middle_start,
                       middle_end);
        draw_gradient (cr,
                       x,
                       y + 2,
                       rect_width - 4,
                       inner_radius,
                       inner_start,
                       inner_end);

	// play/pause-background
        draw_circle (cr,
		     x + rect_width / 2.0f - 2.0f * outter_radius - 4.5f,
		     y - ((circle_radius - outter_radius)),
		     circle_radius,
		     outter_start,
		     outter_end);
        draw_circle (cr,
                       x + rect_width / 2.0f - 2.0f * outter_radius - 4.5f + 1.0f,
                       y - ((circle_radius - outter_radius)) + 1.0f,
                       circle_radius - 1,
                       middle_start,
                       middle_end);
        draw_circle (cr,
                     x + rect_width / 2.0f - 2.0f * outter_radius - 4.5f + 2.0f,
                     y - ((circle_radius - outter_radius)) + 2.0f,
                     circle_radius - 2.0f,
                     inner_start,
		     inner_end);

	// Draw the outside drop shadow background
	//play_button_draw_background_shadow_1(button, cr, x, y, rect_width, rect_height, p_radius);
	
	// Draw the inside drop shadow background
	/*gint offset = 4;
	play_button_draw_background_shadow_2(button, cr, x + offset-1, y + offset/2, rect_width-offset, rect_height-offset, p_radius-(offset/2));*/

	//offset = 5;
	// Draw the inside actual background
	/*play_button_draw_background(button, cr, x+offset-1, y + offset/2+1, rect_width-offset, rect_height-offset, p_radius-offset/2);

	play_button_draw_pause_symbol(cr, rect_width/2 + rect_height/10 + x -1 + offset/2, rect_height/5 +y );

	play_button_draw_pause_symbol(cr, rect_width/2 + rect_height/10 + x + offset/2 + 10, rect_height/5 +y );
	play_button_draw_previous_symbol(cr, x+rect_height/2 + offset, y + offset+2);*/
	cairo_surface_write_to_png(cairo_get_target (cr), "/tmp/foobar.png");	
}

static void 
play_button_draw_previous_symbol(cairo_t* cr, double x, double y)
{
	gint line_width = 2;
	cairo_set_line_width (cr, line_width);
	gint shape_height = 15;
	gint shape_width = 22;
	gint single_arrow_width=16;

	cairo_pattern_t *pat;
	pat = cairo_pattern_create_linear (x, y,  x, y+shape_height);

	cairo_pattern_add_color_stop_rgb(pat, 0, 227/255.0, 222/255.0, 214/255.0);
	cairo_pattern_add_color_stop_rgb(pat, .1, 207/255.0, 201/255.0, 190/255.0);
	cairo_pattern_add_color_stop_rgb(pat, .4, 123/255.0, 123/255.0, 120/255.0);
	cairo_set_source (cr, pat);	

	cairo_move_to(cr, x, y);
	cairo_rel_line_to (cr, 0, shape_height);
	cairo_stroke(cr);
	
	cairo_move_to (cr, x+line_width, y+shape_height/2);	
	cairo_line_to (cr, x+single_arrow_width, y);
	cairo_line_to (cr, x+single_arrow_width, y+shape_height);
	cairo_line_to (cr, x+line_width, y+shape_height/2);
	cairo_fill(cr);
	
	cairo_pattern_destroy (pat);
	cairo_close_path(cr);		
}

static void 
play_button_draw_pause_symbol(cairo_t* cr, double x, double y)
{
	cairo_set_line_width (cr, 7);
	cairo_set_line_cap  (cr, CAIRO_LINE_CAP_ROUND);
	cairo_move_to (cr, x, y);
	cairo_rel_line_to (cr, 0, 18);
	//cairo_set_source_rgb(cr, 94/255.0, 93/255.0, 90/255.0);

	cairo_pattern_t *pat;
	pat = cairo_pattern_create_linear (x, y,  x, y+16);
	cairo_pattern_add_color_stop_rgb(pat, 0, 227/255.0, 222/255.0, 214/255.0);
	cairo_pattern_add_color_stop_rgb(pat, .1, 207/255.0, 201/255.0, 190/255.0);
	cairo_pattern_add_color_stop_rgb(pat, .6, 123/255.0, 123/255.0, 120/255.0);
	cairo_set_source (cr, pat);	
	cairo_stroke(cr);
	cairo_close_path(cr);

	cairo_set_line_width (cr, 5.5);
	cairo_set_line_cap  (cr, CAIRO_LINE_CAP_ROUND);
	cairo_move_to (cr, x, y+0.75);
	cairo_rel_line_to (cr, 0, 16.5);

	pat = cairo_pattern_create_linear (x+1, y+1,  x+1, y+16);
	cairo_pattern_add_color_stop_rgb(pat, .3, 252/255.0, 252/255.0, 251/255.0);
	cairo_pattern_add_color_stop_rgb(pat, .8, 227/255.0, 222/255.0, 214/255.0);
	//cairo_pattern_add_color_stop_rgb(pat, .9, 207/255.0, 201/255.0, 190/255.0);
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
	cairo_pattern_add_color_stop_rgb(pat, .5, 225/255.0, 218/255.0, 211/255.0);
	cairo_pattern_add_color_stop_rgb(pat, .7, 197/255.0, 192/255.0, 185/255.0);
	cairo_pattern_add_color_stop_rgb(pat, .9, 185/255.0, 179/255.0, 173/255.0);
	cairo_set_source (cr, pat);
	cairo_stroke (cr);

	cairo_close_path(cr);
	cairo_arc(cr, rect_width/2 + radius/2 + x + 2, rect_height/2 +y, p_radius, 0, 2*M_PI);
	cairo_set_source (cr, pat);	
	cairo_fill(cr);

	cairo_close_path(cr);
	cairo_arc(cr, rect_width/2 + radius/2 + x + 2, rect_height/2 +y, p_radius, 0, 2*M_PI);
	cairo_set_source_rgb(cr, 94/255.0, 93/255.0, 90/255.0);
	cairo_set_line_width (cr, 0.75);	
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
	pat = cairo_pattern_create_linear (x, y,  x, y+rect_height);
	cairo_pattern_add_color_stop_rgb(pat, .4, 36/255.0, 35/255.0, 33/255.0);
	cairo_pattern_add_color_stop_rgb(pat, .7, 123/255.0, 123/255.0, 120/255.0);
	cairo_set_source (cr, pat);
	cairo_stroke (cr);

	cairo_close_path(cr);
	cairo_arc(cr, rect_width/2 + radius/2 + x + 2, rect_height/2 +y, p_radius, 0, 2*M_PI);
	pat = cairo_pattern_create_linear ((rect_width/2 + radius/2 + x),  rect_height/2 + y-p_radius, (rect_width/2 + radius/2 + x), rect_height+(p_radius-rect_height/2));
	cairo_pattern_add_color_stop_rgb(pat, .4, 36/255.0, 35/255.0, 33/255.0);
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
	pat = cairo_pattern_create_linear (x, y,  x, y+rect_height);
	cairo_pattern_add_color_stop_rgb(pat, .4, 61/255.0, 60/255.0, 57/255.0);
	cairo_pattern_add_color_stop_rgb(pat, .7, 94/255.0, 93/255.0, 90/255.0);
	cairo_set_source (cr, pat);
	cairo_stroke (cr);
	
	cairo_close_path(cr);
	cairo_arc(cr, rect_width/2 + radius/2 + x + 2, rect_height/2 +y, p_radius, 0, 2*M_PI);
	pat = cairo_pattern_create_linear ((rect_width/2 + radius/2 + x),  rect_height/2 + y-p_radius, (rect_width/2 + radius/2 + x), rect_height+(p_radius-rect_height/2));
	cairo_pattern_add_color_stop_rgb(pat, .4, 61/255.0, 60/255.0, 57/255.0);
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

