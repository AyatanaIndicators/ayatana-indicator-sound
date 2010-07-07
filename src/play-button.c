/*
Copyright 2010 Canonical Ltd.

Authors:
    Conor Curran <conor.curran@canonical.com>
    Mirco Müller <mirco.mueller@canonical.com>

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
_setup (cairo_t**         cr,
	cairo_surface_t** surf,
	gint              width,
	gint              height)
{
	if (!cr || !surf)
		return;

	*surf = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
	*cr = cairo_create (*surf);
	cairo_scale (*cr, 1.0f, 1.0f);
	cairo_set_operator (*cr, CAIRO_OPERATOR_CLEAR);
	cairo_paint (*cr);
	cairo_set_operator (*cr, CAIRO_OPERATOR_OVER);
}

static void
_mask_prev (cairo_t* cr,
	    double   x,
	    double   y,
	    double   tri_width,
	    double   tri_height,
	    double   tri_offset)
{
	if (!cr)
		return;

	cairo_move_to (cr, x,             y + tri_height / 2.0f);
	cairo_line_to (cr, x + tri_width, y);
	cairo_line_to (cr, x + tri_width, y + tri_height);
	x += tri_offset;
	cairo_move_to (cr, x,             y + tri_height / 2.0f);
	cairo_line_to (cr, x + tri_width, y);
	cairo_line_to (cr, x + tri_width, y + tri_height);
	x -= tri_offset;
	cairo_rectangle (cr, x, y, 2.5f, tri_height);
	cairo_close_path (cr);	
}

static void
_mask_next (cairo_t* cr,
	    double   x,
	    double   y,
	    double   tri_width,
	    double   tri_height,
	    double   tri_offset)
{
	if (!cr)
		return;

	cairo_move_to (cr, x,             y);
	cairo_line_to (cr, x + tri_width, y + tri_height / 2.0f);
	cairo_line_to (cr, x,             y + tri_height);
	x += tri_offset;
	cairo_move_to (cr, x,             y);
	cairo_line_to (cr, x + tri_width, y + tri_height / 2.0f);
	cairo_line_to (cr, x,             y + tri_height);
	x -= tri_offset;
	x += 2.0f * tri_width - tri_offset - 1.0f;
	cairo_rectangle (cr, x, y, 2.5f, tri_height);

	cairo_close_path (cr);	
}

static void
_mask_pause (cairo_t* cr,
	     double   x,
	     double   y,
	     double   bar_width,
	     double   bar_height,
	     double   bar_offset)
{
	if (!cr)
		return;

	cairo_set_line_width (cr, bar_width);
	cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);

	x += bar_width;
	y += bar_width;
	cairo_move_to (cr, x,              y);
	cairo_line_to (cr, x,              y + bar_height);
	cairo_move_to (cr, x + bar_offset, y);
	cairo_line_to (cr, x + bar_offset, y + bar_height);

	//cairo_close_path (cr);
}

static void
_fill (cairo_t* cr,
       double   x_start,
       double   y_start,
       double   x_end,
       double   y_end,
       double*  rgba_start,
       double*  rgba_end,
       gboolean stroke)
{
	cairo_pattern_t* pattern = NULL;

	if (!cr || !rgba_start || !rgba_end)
		return;

	pattern = cairo_pattern_create_linear (x_start, y_start, x_end, y_end);
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
	if (stroke)
		cairo_stroke (cr);
	else
		cairo_fill (cr);
	cairo_pattern_destroy (pattern);
}

static void
_finalize (cairo_t*          cr,
	   cairo_t**         cr_surf,
	   cairo_surface_t** surf,
	   double            x,
	   double            y)
{
	if (!cr || !cr_surf || !surf)
		return;

	cairo_set_source_surface (cr, *surf, x, y);
	cairo_paint (cr);
	cairo_surface_destroy (*surf);
	cairo_destroy (*cr_surf);
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

	double button_start[]  = {252.0f / 255.0f,
				  251.0f / 255.0f,
				  251.0f / 255.0f,
				  1.0f};
	double button_end[]    = {186.0f / 255.0f,
				  180.0f / 255.0f,
				  170.0f / 255.0f,
				  1.0f};
	double button_shadow[] = {0.0f / 255.0f,
				  0.0f / 255.0f,
				  0.0f / 255.0f,
				  0.75f};
	double prev_width      = 25.0f;
	double prev_height     = 17.0f;
	double next_width      = prev_width;
	double next_height     = prev_height;
	cairo_surface_t*  surf = NULL;
	cairo_t*       cr_surf = NULL;
	double tri_width       = 11.0f;
	double tri_height      = 13.0f;
	double tri_offset      =  6.0f;
	double prev_x          = 20.0f;
	double prev_y          = 21.0f;
	double next_x          = 98.0f;
	double next_y          = prev_y;
	double pause_width     = 21.0f;
	double pause_height    = 27.0f;
	double bar_width       = 4.5f;
	double bar_height      = 24.0f;
	double bar_offset      = 10.0f;
	double pause_x         = 62.0f;
	double pause_y         = 15.0f;

	// prev/next-background
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

	// draw previous-button drop-shadow
	_setup (&cr_surf, &surf, prev_width, prev_height);
	_mask_prev (cr_surf,
		    (prev_width - (2.0f * tri_width - tri_offset)) / 2.0f,
		    (prev_height - tri_height) / 2.0f,
		    tri_width,
		    tri_height,
		    tri_offset);
	_fill (cr_surf,
               (prev_width - (2.0f * tri_width - tri_offset)) / 2.0f,
               (prev_height - tri_height) / 2.0f,
               (prev_width - (2.0f * tri_width - tri_offset)) / 2.0f,
	       (double) tri_height,
	       button_shadow,
	       button_shadow,
	       FALSE);
	_surface_blur (surf, 1);
	_finalize (cr, &cr_surf, &surf, prev_x, prev_y + 1.0f);

	// draw previous-button
	_setup (&cr_surf, &surf, prev_width, prev_height);
	_mask_prev (cr_surf,
		    (prev_width - (2.0f * tri_width - tri_offset)) / 2.0f,
		    (prev_height - tri_height) / 2.0f,
		    tri_width,
		    tri_height,
		    tri_offset);
	_fill (cr_surf,
	       (prev_width - (2.0f * tri_width - tri_offset)) / 2.0f,
	       (prev_height - tri_height) / 2.0f,
	       (prev_width - (2.0f * tri_width - tri_offset)) / 2.0f,
	       (double) tri_height,
	       button_start,
	       button_end,
	       FALSE);
	_finalize (cr, &cr_surf, &surf, prev_x, prev_y);

	// draw next-button drop-shadow
	_setup (&cr_surf, &surf, next_width, next_height);
	_mask_next (cr_surf,
		    (next_width - (2.0f * tri_width - tri_offset)) / 2.0f,
		    (next_height - tri_height) / 2.0f,
		    tri_width,
		    tri_height,
		    tri_offset);
	_fill (cr_surf,
	       (next_width - (2.0f * tri_width - tri_offset)) / 2.0f,
	       (next_height - tri_height) / 2.0f,
	       (next_width - (2.0f * tri_width - tri_offset)) / 2.0f,
	       (double) tri_height,
	       button_shadow,
	       button_shadow,
	       FALSE);
	_surface_blur (surf, 1);
	_finalize (cr, &cr_surf, &surf, next_x, next_y + 1.0f);

	// draw next-button
	_setup (&cr_surf, &surf, next_width, next_height);
	_mask_next (cr_surf,
		    (next_width - (2.0f * tri_width - tri_offset)) / 2.0f,
		    (next_height - tri_height) / 2.0f,
		    tri_width,
		    tri_height,
		    tri_offset);
	_fill (cr_surf,
	       (next_width - (2.0f * tri_width - tri_offset)) / 2.0f,
	       (next_height - tri_height) / 2.0f,
	       (next_width - (2.0f * tri_width - tri_offset)) / 2.0f,
	       (double) tri_height,
	       button_start,
	       button_end,
	       FALSE);
	_finalize (cr, &cr_surf, &surf, next_x, next_y);

	// draw pause-button drop-shadow
	_setup (&cr_surf, &surf, pause_width, pause_height);
	_mask_pause (cr_surf,
		     (pause_width - (2.0f * bar_width + bar_offset)) / 2.0f,
		     (pause_height - bar_height) / 2.0f,
		     bar_width,
		     bar_height - 2.0f * bar_width,
		     bar_offset);
	_fill (cr_surf,
	       (pause_width - (2.0f * bar_width + bar_offset)) / 2.0f,
	       (pause_height - bar_height) / 2.0f,
	       (pause_width - (2.0f * bar_width + bar_offset)) / 2.0f,
	       (double) bar_height,
	       button_shadow,
	       button_shadow,
	       TRUE);
	_surface_blur (surf, 1);
	_finalize (cr, &cr_surf, &surf, pause_x, pause_y + 1.0f);

	// draw pause-button
	_setup (&cr_surf, &surf, pause_width, pause_height);
	_mask_pause (cr_surf,
		     (pause_width - (2.0f * bar_width + bar_offset)) / 2.0f,
		     (pause_height - bar_height) / 2.0f,
		     bar_width,
		     bar_height - 2.0f * bar_width,
		     bar_offset);
	_fill (cr_surf,
	       (pause_width - (2.0f * bar_width + bar_offset)) / 2.0f,
	       (pause_height - bar_height) / 2.0f,
	       (pause_width - (2.0f * bar_width + bar_offset)) / 2.0f,
	       (double) bar_height,
	       button_start,
	       button_end,
	       TRUE);
	_finalize (cr, &cr_surf, &surf, pause_x, pause_y);

	cairo_surface_write_to_png (cairo_get_target (cr), "/tmp/foobar.png");
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

