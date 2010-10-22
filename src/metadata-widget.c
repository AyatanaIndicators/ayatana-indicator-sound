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
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib/gi18n.h>
#include "metadata-widget.h"
#include "common-defs.h"
#include <gtk/gtk.h>
#include <glib.h>
#include "transport-widget.h"

typedef struct _MetadataWidgetPrivate MetadataWidgetPrivate;

struct _MetadataWidgetPrivate
{
  gboolean   theme_change_occured;
	GtkWidget* hbox;
	GtkWidget* album_art;
  GString*	 image_path;
  GString*	 old_image_path;
	GtkWidget* artist_label;
	GtkWidget* piece_label;
	GtkWidget* container_label;
  DbusmenuMenuitem* twin_item;		  
  gint artwork_offset;
};

#define METADATA_WIDGET_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), METADATA_WIDGET_TYPE, MetadataWidgetPrivate))

/* Prototypes */
static void metadata_widget_class_init    (MetadataWidgetClass *klass);
static void metadata_widget_init          (MetadataWidget *self);
static void metadata_widget_dispose       (GObject *object);
static void metadata_widget_finalize      (GObject *object);
static gboolean metadata_image_expose     (GtkWidget *image, GdkEventExpose *event, gpointer user_data);
static void metadata_widget_set_style     (GtkWidget* button, GtkStyle* style);
static void metadata_widget_set_twin_item (MetadataWidget* self, DbusmenuMenuitem* twin_item);

// keyevent consumers
static gboolean metadata_widget_button_press_event (GtkWidget *menuitem, 
                                  									GdkEventButton *event);
// Dbusmenuitem properties update callback
static void metadata_widget_property_update (DbusmenuMenuitem* item,
                                             gchar* property, 
                                       			 GValue* value,
                                             gpointer userdata);
static void metadata_widget_style_labels ( MetadataWidget* self,
                                           GtkLabel* label);
static void draw_album_art_placeholder ( GtkWidget *metadata);
static void draw_album_border ( GtkWidget *metadata);

G_DEFINE_TYPE (MetadataWidget, metadata_widget, GTK_TYPE_MENU_ITEM);


static void
metadata_widget_class_init (MetadataWidgetClass *klass)
{
	GObjectClass 			*gobject_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass    *widget_class = GTK_WIDGET_CLASS (klass);

	widget_class->button_press_event = metadata_widget_button_press_event;
	
	g_type_class_add_private (klass, sizeof (MetadataWidgetPrivate));

	gobject_class->dispose = metadata_widget_dispose;
	gobject_class->finalize = metadata_widget_finalize;
}

static void
metadata_widget_init (MetadataWidget *self)
{
	//g_debug("MetadataWidget::metadata_widget_init");

	MetadataWidgetPrivate * priv = METADATA_WIDGET_GET_PRIVATE(self);
	GtkWidget *hbox;

	hbox = gtk_hbox_new(FALSE, 0);
	priv->hbox = hbox;

	// image
	priv->album_art = gtk_image_new();
	priv->image_path = g_string_new(dbusmenu_menuitem_property_get(priv->twin_item, DBUSMENU_METADATA_MENUITEM_ARTURL));
	priv->old_image_path = g_string_new("");
  priv->artwork_offset = 2;
	//g_debug("Metadata::At startup and image path = %s", priv->image_path->str);
	
  g_signal_connect(priv->album_art, "expose-event", 
                   G_CALLBACK(metadata_image_expose),
                   GTK_WIDGET(self));		
	gtk_widget_set_size_request(GTK_WIDGET(priv->album_art), 60, 60); 
	
	gtk_box_pack_start (GTK_BOX (priv->hbox),
                      priv->album_art,
                      FALSE,
                      FALSE,
                      priv->artwork_offset * 2);	
	
  priv->theme_change_occured = FALSE;

  GtkWidget* vbox = gtk_vbox_new(FALSE, 0);
	
	// artist
	GtkWidget* artist;
	artist = gtk_label_new(dbusmenu_menuitem_property_get(priv->twin_item,
	                                                      DBUSMENU_METADATA_MENUITEM_ARTIST));
	gtk_misc_set_alignment(GTK_MISC(artist), (gfloat)0, (gfloat)0);
	gtk_misc_set_padding (GTK_MISC(artist), (gfloat)10, (gfloat)0);
  gtk_widget_set_size_request (artist, 140, 15);
  gtk_label_set_ellipsize(GTK_LABEL(artist), PANGO_ELLIPSIZE_MIDDLE);	
	metadata_widget_style_labels(self, GTK_LABEL(artist));
	priv->artist_label = artist;
	
	// title
	GtkWidget* piece;
	piece = gtk_label_new(dbusmenu_menuitem_property_get( priv->twin_item,
	                                                      DBUSMENU_METADATA_MENUITEM_TITLE) );
	gtk_misc_set_alignment(GTK_MISC(piece), (gfloat)0, (gfloat)0);
	gtk_misc_set_padding (GTK_MISC(piece), (gfloat)10, (gfloat)0);
  gtk_widget_set_size_request (piece, 140, 15);
  gtk_label_set_ellipsize(GTK_LABEL(piece), PANGO_ELLIPSIZE_MIDDLE);
	metadata_widget_style_labels(self, GTK_LABEL(piece));
	priv->piece_label =  piece;

	// container
	GtkWidget* container;
	container = gtk_label_new(dbusmenu_menuitem_property_get( priv->twin_item,
	                                                          DBUSMENU_METADATA_MENUITEM_ALBUM) );
	gtk_misc_set_alignment(GTK_MISC(container), (gfloat)0, (gfloat)0);
	gtk_misc_set_padding (GTK_MISC(container), (gfloat)10, (gfloat)0);	
  gtk_widget_set_size_request (container, 140, 15);
	gtk_label_set_ellipsize(GTK_LABEL(container), PANGO_ELLIPSIZE_MIDDLE);	
	metadata_widget_style_labels(self, GTK_LABEL(container));
	priv->container_label = container;

	gtk_box_pack_start (GTK_BOX (vbox), priv->piece_label, FALSE, FALSE, 0);	
	gtk_box_pack_start (GTK_BOX (vbox), priv->artist_label, FALSE, FALSE, 0);	
	gtk_box_pack_start (GTK_BOX (vbox), priv->container_label, FALSE, FALSE, 0);	
	
	gtk_box_pack_start (GTK_BOX (priv->hbox), vbox, FALSE, FALSE, 0);	

	gtk_widget_show_all (priv->hbox);

  g_signal_connect(self, "style-set", G_CALLBACK(metadata_widget_set_style), GTK_WIDGET(self));		
	
	gtk_widget_set_size_request(GTK_WIDGET(self), 200, 65); 
  gtk_container_add (GTK_CONTAINER (self), hbox);
}

static void
metadata_widget_dispose (GObject *object)
{
	G_OBJECT_CLASS (metadata_widget_parent_class)->dispose (object);
}

static void
metadata_widget_finalize (GObject *object)
{
	G_OBJECT_CLASS (metadata_widget_parent_class)->finalize (object);
}


/**
 * We override the expose method to enable primitive drawing of the 
 * empty album art image and rounded rectangles on the album art.
 */
static gboolean
metadata_image_expose (GtkWidget *metadata, GdkEventExpose *event, gpointer user_data)
{
	g_return_val_if_fail(IS_METADATA_WIDGET(user_data), FALSE);
	MetadataWidget* widget = METADATA_WIDGET(user_data);
	MetadataWidgetPrivate * priv = METADATA_WIDGET_GET_PRIVATE(widget);	  
  draw_album_border(metadata);  
	if(priv->image_path->len > 0){
	  if(g_string_equal(priv->image_path, priv->old_image_path) == FALSE ||
       priv->theme_change_occured == TRUE){
      priv->theme_change_occured = FALSE;         
			GdkPixbuf* pixbuf;
			pixbuf = gdk_pixbuf_new_from_file(priv->image_path->str, NULL);
			//g_debug("metadata_load_new_image -> pixbuf from %s",
			//				priv->image_path->str); 
			if(GDK_IS_PIXBUF(pixbuf) == FALSE){
				//g_debug("problem loading the downloaded image just use the placeholder instead");
				draw_album_art_placeholder(metadata);
				return TRUE;				
			}         
			pixbuf = gdk_pixbuf_scale_simple(pixbuf,60, 60, GDK_INTERP_BILINEAR);
    	gtk_image_set_from_pixbuf(GTK_IMAGE(priv->album_art), pixbuf);
			g_string_erase(priv->old_image_path, 0, -1);
			g_string_overwrite(priv->old_image_path, 0, priv->image_path->str); 

			g_object_unref(pixbuf);				
		}
		return FALSE;				
	}
	draw_album_art_placeholder(metadata);
	return TRUE;
}


static void
draw_gradient (cairo_t* cr,
               GtkAllocation alloc,
               double*  rgba_start,
               double*  rgba_end)
{
	cairo_pattern_t* pattern = NULL;
  cairo_rectangle (cr,
                   alloc.x, alloc.y,
                   alloc.width, alloc.height);
  
	cairo_clip(cr);

	cairo_move_to (cr, alloc.x , alloc.y);
	cairo_line_to(cr, alloc.x + alloc.width,
	              alloc.y);
	cairo_line_to(cr, alloc.x + alloc.width,
	              alloc.y + alloc.height);
	cairo_line_to(cr, alloc.x, alloc.y + alloc.height);
	cairo_line_to(cr, alloc.x, alloc.y);

	cairo_close_path (cr);

  cairo_set_line_width (cr, 3.0);
  CairoColorRGB darkened_top_color;
  
  _color_shade (&rgba_end, 0.8, &color_button[2]);
  
	pattern = cairo_pattern_create_linear (alloc.x, 
                                         alloc.y,
                                         alloc.x,
                                         alloc.y + alloc.height);
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
	cairo_stroke (cr);
	cairo_pattern_destroy (pattern);
}

static void
draw_album_border(GtkWidget *metadata)
{
	cairo_t *cr;	
	cr = gdk_cairo_create (metadata->window);
  GtkStyle *style;
	style = gtk_widget_get_style (metadata);
  
	GtkAllocation alloc;
	gtk_widget_get_allocation (metadata, &alloc);
  gint offset = 2;
  
  alloc.width = alloc.width + (offset * 2);
  alloc.height = alloc.height + (offset * 2);
  alloc.x = alloc.x - offset;
  alloc.y = alloc.y - offset;

  double start_colour[] = { style->bg[0].red/65535.0,
                            style->bg[0].green/65535.0,
                            style->bg[0].blue/65535.0,
                            1.0f  };

  double end_colour[] = {   style->fg[0].red/65535.0,
                            style->fg[0].green/65535.0,
                            style->fg[0].blue/65535.0,
                            1.0f};

  draw_gradient(cr,
                alloc,
                start_colour,
                end_colour);
                  
	/*cairo_rectangle (cr,
                   alloc.x, alloc.y,
                   alloc.width, alloc.height);
	cairo_clip(cr);

	cairo_move_to (cr, alloc.x , alloc.y);
	cairo_line_to(cr, alloc.x + alloc.width,
	              alloc.y);
	cairo_line_to(cr, alloc.x + alloc.width,
	              alloc.y + alloc.height);
	cairo_line_to(cr, alloc.x, alloc.y + alloc.height);
	cairo_line_to(cr, alloc.x, alloc.y);


	cairo_close_path (cr);

  cairo_set_source_rgba (cr,
                         style->fg[0].red/65535.0, 
                         style->fg[0].green/65535.0,
                         style->fg[0].blue/65535.0,
                         0.6);
	cairo_set_line_width (cr, 2.0);
	
	cairo_stroke (cr);*/						  
}

static void
draw_album_art_placeholder(GtkWidget *metadata)
{		
	cairo_t *cr;	
	cr = gdk_cairo_create (metadata->window);
  GtkStyle *style;
	style = gtk_widget_get_style (metadata);
  
	GtkAllocation alloc;
	gtk_widget_get_allocation (metadata, &alloc);
  
	// Draw the eight note
	PangoLayout *layout;
	PangoFontDescription *desc;
	layout = pango_cairo_create_layout(cr);
	PangoContext* pcontext = pango_cairo_create_context(cr); 
	pango_cairo_context_set_resolution (pcontext, 96);

	GString* string = g_string_new("");
	gssize size = -1;
	gunichar code = g_utf8_get_char_validated("\342\231\253", size);	
	g_string_append_unichar (string, code);
	
	pango_layout_set_text(layout, string->str, -1);
	desc = pango_font_description_from_string("Sans Bold 30");
	pango_layout_set_font_description(layout, desc);
	pango_font_description_free(desc);

  cairo_set_source_rgba (cr,
                         style->fg[0].red/65535.0, 
                         style->fg[0].green/65535.0,
                         style->fg[0].blue/65535.0,
                         0.8);
  
	pango_cairo_update_layout(cr, layout);
	cairo_move_to (cr, alloc.x + alloc.width/6, alloc.y);	
	pango_cairo_show_layout(cr, layout);

	g_object_unref(layout);	
	g_object_unref(pcontext);
	g_string_free (string, TRUE);
	cairo_destroy (cr);	

}

/* Suppress/consume keyevents */
static gboolean
metadata_widget_button_press_event (GtkWidget *menuitem, 
                                  GdkEventButton *event)
{
	GtkClipboard* board = gtk_clipboard_get (GDK_NONE);	

  MetadataWidgetPrivate* priv = METADATA_WIDGET_GET_PRIVATE(METADATA_WIDGET(menuitem));	
  
	gchar* contents = g_strdup_printf("artist: %s \ntitle: %s \nalbum: %s",
                                    dbusmenu_menuitem_property_get(priv->twin_item,
	                                                      DBUSMENU_METADATA_MENUITEM_ARTIST),
                                    dbusmenu_menuitem_property_get(priv->twin_item,
	                                                      DBUSMENU_METADATA_MENUITEM_TITLE),
                                    dbusmenu_menuitem_property_get(priv->twin_item,
	                                                      DBUSMENU_METADATA_MENUITEM_ALBUM));
	//g_debug("contents to be copied will be : %s", contents);	
	gtk_clipboard_set_text (board, contents, -1);
	gtk_clipboard_store (board);
	g_free(contents);
	return FALSE;
}

// TODO: Manage empty/mangled music details <unknown artist> etc.
static void 
metadata_widget_property_update(DbusmenuMenuitem* item, gchar* property, 
                                       GValue* value, gpointer userdata)
{
	g_return_if_fail (IS_METADATA_WIDGET (userdata));	

	if(g_value_get_int(value) == DBUSMENU_PROPERTY_EMPTY){
		//g_debug("Metadata widget: property update - reset");
		GValue new_value = {0};
  	g_value_init(&new_value, G_TYPE_STRING);		
		g_value_set_string(&new_value, g_strdup(""));		
		value = &new_value;
	}
	
	MetadataWidget* mitem = METADATA_WIDGET(userdata);
	MetadataWidgetPrivate * priv = METADATA_WIDGET_GET_PRIVATE(mitem);
	
	if(g_ascii_strcasecmp(DBUSMENU_METADATA_MENUITEM_ARTIST, property) == 0){  
		gtk_label_set_text(GTK_LABEL(priv->artist_label), g_value_get_string(value));
		metadata_widget_style_labels(mitem, GTK_LABEL(priv->artist_label));
	}
	else if(g_ascii_strcasecmp(DBUSMENU_METADATA_MENUITEM_TITLE, property) == 0){  
		gtk_label_set_text(GTK_LABEL(priv->piece_label), g_value_get_string(value));		
		metadata_widget_style_labels(mitem, GTK_LABEL(priv->piece_label));
	}	
	else if(g_ascii_strcasecmp(DBUSMENU_METADATA_MENUITEM_ALBUM, property) == 0){  
		gtk_label_set_text(GTK_LABEL(priv->container_label), g_value_get_string(value));
		metadata_widget_style_labels(mitem, GTK_LABEL(priv->container_label));
	}	
	else if(g_ascii_strcasecmp(DBUSMENU_METADATA_MENUITEM_ARTURL, property) == 0){
		g_string_erase(priv->image_path, 0, -1);
		g_string_overwrite(priv->image_path, 0, g_value_get_string (value));
		// if its a remote image queue a redraw incase the download took too long
		if (g_str_has_prefix(g_value_get_string (value), g_get_user_cache_dir())){
			//g_debug("the image update is a download so redraw");
			gtk_widget_queue_draw(GTK_WIDGET(mitem));
		}
	}		
}


static cairo_surface_t *
surface_from_pixbuf (GdkPixbuf *pixbuf)
{
        cairo_surface_t *surface;
        cairo_t         *cr;

        surface = cairo_image_surface_create (gdk_pixbuf_get_has_alpha (pixbuf) ?
                                              CAIRO_FORMAT_ARGB32 : CAIRO_FORMAT_RGB24,
                                              gdk_pixbuf_get_width (pixbuf),
                                              gdk_pixbuf_get_height (pixbuf));
        cr = cairo_create (surface);
        gdk_cairo_set_source_pixbuf (cr, pixbuf, 0, 0);
        cairo_paint (cr);
        cairo_destroy (cr);

        return surface;
}

static void
rounded_rectangle (cairo_t *cr,
                   gdouble  aspect,
                   gdouble  x,
                   gdouble  y,
                   gdouble  corner_radius,
                   gdouble  width,
                   gdouble  height)
{
        gdouble radius;
        gdouble degrees;
				
        radius = corner_radius / aspect;
        degrees = G_PI / 180.0;
        cairo_new_sub_path (cr);
        cairo_arc (cr,
                   x + width - radius,
                   y + radius,
                   radius,
                   -90 * degrees,
                   0 * degrees);
        cairo_arc (cr,
                   x + width - radius,
                   y + height - radius,
                   radius,
                   0 * degrees,
                   90 * degrees);
        cairo_arc (cr,
                   x + radius,
                   y + height - radius,
                   radius,
                   90 * degrees,
                   180 * degrees);
        cairo_arc (cr,
                   x + radius,
                   y + radius,
                   radius,
                   180 * degrees,
                   270 * degrees);
	
        cairo_close_path (cr);
}


static void
metadata_widget_style_labels(MetadataWidget* self, GtkLabel* label)
{
	char* markup;
	markup = g_markup_printf_escaped ("<span size=\"smaller\">%s</span>",
	                                  gtk_label_get_text(GTK_LABEL(label)));
	gtk_label_set_markup (GTK_LABEL (label), markup);
	g_free(markup);	
}

static void
metadata_widget_set_style(GtkWidget* metadata, GtkStyle* style)
{
	g_return_if_fail(IS_METADATA_WIDGET(metadata));
	MetadataWidget* widg = METADATA_WIDGET(metadata);
	MetadataWidgetPrivate * priv = METADATA_WIDGET_GET_PRIVATE(widg);	
  priv->theme_change_occured = TRUE;    
	gtk_widget_queue_draw(GTK_WIDGET(metadata));  
}

static void
metadata_widget_set_twin_item(MetadataWidget* self,
           								    DbusmenuMenuitem* twin_item)
{
    MetadataWidgetPrivate* priv = METADATA_WIDGET_GET_PRIVATE(self);
    priv->twin_item = twin_item;
    g_signal_connect(G_OBJECT(priv->twin_item), "property-changed", 
                              G_CALLBACK(metadata_widget_property_update), self);
}

 /**
 * transport_new:
 * @returns: a new #MetadataWidget.
 **/
GtkWidget* 
metadata_widget_new(DbusmenuMenuitem *item)
{

  GtkWidget* widget =	 g_object_new(METADATA_WIDGET_TYPE, NULL);
  metadata_widget_set_twin_item ( METADATA_WIDGET(widget),
                                  item );
  return widget;                  
}

