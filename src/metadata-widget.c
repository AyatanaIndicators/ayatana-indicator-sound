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
#include "metadata-widget.h"
#include "common-defs.h"
#include <gtk/gtk.h>

static DbusmenuMenuitem* twin_item;

typedef struct _MetadataWidgetPrivate MetadataWidgetPrivate;

struct _MetadataWidgetPrivate
{
	GtkWidget* hbox;
	GtkWidget* album_art;
  gchar* 		 image_path;
	GtkWidget* artist_label;
	GtkWidget* piece_label;
	GtkWidget* container_label;	
};

#define METADATA_WIDGET_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), METADATA_WIDGET_TYPE, MetadataWidgetPrivate))

/* Prototypes */
static void metadata_widget_class_init (MetadataWidgetClass *klass);
static void metadata_widget_init       (MetadataWidget *self);
static void metadata_widget_dispose    (GObject *object);
static void metadata_widget_finalize   (GObject *object);
static gboolean metadata_widget_expose_event(GtkWidget* widget, GdkEventExpose* event);

// keyevent consumers
static gboolean metadata_widget_button_press_event (GtkWidget *menuitem, 
                                  									GdkEventButton *event);
static gboolean metadata_widget_button_release_event (GtkWidget *menuitem, 
                                  										GdkEventButton *event);
// Dbusmenuitem properties update callback
static void metadata_widget_property_update(DbusmenuMenuitem* item, gchar* property, 
                                       GValue* value, gpointer userdata);

static void update_album_art(MetadataWidget* self);
static void style_artist_text(MetadataWidget* self);
static void style_title_text(MetadataWidget* self);
static void style_album_text(MetadataWidget* self);


G_DEFINE_TYPE (MetadataWidget, metadata_widget, GTK_TYPE_MENU_ITEM);



static void
metadata_widget_class_init (MetadataWidgetClass *klass)
{
	GObjectClass 			*gobject_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass    *widget_class = GTK_WIDGET_CLASS (klass);

	widget_class->button_press_event = metadata_widget_button_press_event;
  widget_class->button_release_event = metadata_widget_button_release_event;
	widget_class->expose_event = metadata_widget_expose_event;
	g_type_class_add_private (klass, sizeof (MetadataWidgetPrivate));

	gobject_class->dispose = metadata_widget_dispose;
	gobject_class->finalize = metadata_widget_finalize;

}

static void
metadata_widget_init (MetadataWidget *self)
{
	g_debug("MetadataWidget::metadata_widget_init");

	MetadataWidgetPrivate * priv = METADATA_WIDGET_GET_PRIVATE(self);

	GtkWidget *hbox;

	hbox = gtk_hbox_new(FALSE, 0);
	priv->hbox = hbox;

	// image
	priv->album_art = gtk_image_new();
	priv->image_path = g_strdup(dbusmenu_menuitem_property_get(twin_item, DBUSMENU_METADATA_MENUITEM_ARTURL));
	update_album_art(self);	

	gtk_box_pack_start (GTK_BOX (priv->hbox), priv->album_art, FALSE, FALSE, 0);	

	
	GtkWidget* vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
	// artist
	GtkWidget* artist;
	artist = gtk_label_new(dbusmenu_menuitem_property_get(twin_item,
	                                                      DBUSMENU_METADATA_MENUITEM_ARTIST));
	
	gtk_misc_set_alignment(GTK_MISC(artist), (gfloat)0, (gfloat)0);
	gtk_label_set_width_chars(GTK_LABEL(artist), 15);	
	gtk_label_set_ellipsize(GTK_LABEL(artist), PANGO_ELLIPSIZE_MIDDLE);	
	priv->artist_label = artist;
	// Style it up.
	style_artist_text(self);
	
	// piece
	GtkWidget* piece;
	piece = gtk_label_new(dbusmenu_menuitem_property_get(twin_item,
	                                                     DBUSMENU_METADATA_MENUITEM_TITLE));
	gtk_misc_set_alignment(GTK_MISC(piece), (gfloat)0, (gfloat)0);
	gtk_label_set_width_chars(GTK_LABEL(piece), 12);
	gtk_label_set_ellipsize(GTK_LABEL(piece), PANGO_ELLIPSIZE_MIDDLE);
	priv->piece_label =  piece;
	// Style it up.
	style_title_text(self);

	// container
	GtkWidget* container;
	container = gtk_label_new(dbusmenu_menuitem_property_get(twin_item,
	                                                         DBUSMENU_METADATA_MENUITEM_ALBUM));
	gtk_misc_set_alignment(GTK_MISC(container), (gfloat)0, (gfloat)0);
	gtk_label_set_width_chars(GTK_LABEL(container), 15);		
	gtk_label_set_ellipsize(GTK_LABEL(container), PANGO_ELLIPSIZE_MIDDLE);	
	priv->container_label = container;
	// Style it up.
	style_album_text(self);

	// Pack in the right order
	gtk_box_pack_start (GTK_BOX (vbox), priv->piece_label, FALSE, FALSE, 0);	
	gtk_box_pack_start (GTK_BOX (vbox), priv->artist_label, FALSE, FALSE, 0);	
	gtk_box_pack_start (GTK_BOX (vbox), priv->container_label, FALSE, FALSE, 0);	
	
	gtk_box_pack_start (GTK_BOX (priv->hbox), vbox, FALSE, FALSE, 0);	

	g_signal_connect(G_OBJECT(twin_item), "property-changed", 
	                 G_CALLBACK(metadata_widget_property_update), self);
	gtk_widget_show_all (priv->hbox);
	gtk_widget_set_size_request(GTK_WIDGET(self), 200, 60); 
  gtk_container_add (GTK_CONTAINER (self), hbox);	
}

static gboolean
metadata_widget_expose_event(GtkWidget* widget, GdkEventExpose* event)
{
	MetadataWidgetPrivate * priv = METADATA_WIDGET_GET_PRIVATE(widget);
		
	gtk_container_propagate_expose(GTK_CONTAINER(widget), priv->hbox, event); 
	return TRUE;
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

/* Suppress/consume keyevents */
static gboolean
metadata_widget_button_press_event (GtkWidget *menuitem, 
                                  GdkEventButton *event)
{
	g_debug("MetadataWidget::menu_press_event");
	return TRUE;
}

static gboolean
metadata_widget_button_release_event (GtkWidget *menuitem, 
                                  GdkEventButton *event)
{
	g_debug("MetadataWidget::menu_release_event");
	return TRUE;
}

// TODO: Manage empty/mangled music details <unknown artist> etc.
static void 
metadata_widget_property_update(DbusmenuMenuitem* item, gchar* property, 
                                       GValue* value, gpointer userdata)
{
	g_return_if_fail (IS_METADATA_WIDGET (userdata));	

	if(g_value_get_int(value) == DBUSMENU_PROPERTY_EMPTY){
		g_debug("Metadata widget: property update - reset");
		GValue new_value = {0};
  	g_value_init(&new_value, G_TYPE_STRING);		
		g_value_set_string(&new_value, g_strdup(""));		
		value = &new_value;
	}
	
	MetadataWidget* mitem = METADATA_WIDGET(userdata);
	MetadataWidgetPrivate * priv = METADATA_WIDGET_GET_PRIVATE(mitem);
	
	if(g_ascii_strcasecmp(DBUSMENU_METADATA_MENUITEM_ARTIST, property) == 0){  
		gtk_label_set_text(GTK_LABEL(priv->artist_label), g_value_get_string(value));
		style_artist_text(mitem);
	}
	else if(g_ascii_strcasecmp(DBUSMENU_METADATA_MENUITEM_TITLE, property) == 0){  
		gtk_label_set_text(GTK_LABEL(priv->piece_label), g_value_get_string(value));		
		style_title_text(mitem);
	}	
	else if(g_ascii_strcasecmp(DBUSMENU_METADATA_MENUITEM_ALBUM, property) == 0){  
		gtk_label_set_text(GTK_LABEL(priv->container_label), g_value_get_string(value));
		style_album_text(mitem);	
	}	
	else if(g_ascii_strcasecmp(DBUSMENU_METADATA_MENUITEM_ARTURL, property) == 0){
		if(priv->image_path != NULL){
			g_free(priv->image_path);
		}
		priv->image_path = g_value_dup_string(value);
		if(priv->image_path != NULL){
			update_album_art(mitem);
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
image_set_from_pixbuf (GtkImage  *image,
                       GdkPixbuf *source)
{
        cairo_t         *cr;
        cairo_t         *cr_mask;
        cairo_surface_t *surface;
        GdkPixmap       *pixmap;
        GdkPixmap       *bitmask;
        int              w;
        int              h;
        int              frame_width;
        double           radius;
        GdkColor         color;
        double           r;
        double           g;
        double           b;

        frame_width = 5;

        w = gdk_pixbuf_get_width (source) + frame_width * 2;
        h = gdk_pixbuf_get_height (source) + frame_width * 2;

        radius = w / 10;

        pixmap = gdk_pixmap_new (gtk_widget_get_window (GTK_WIDGET (image)), w, h, -1);
        bitmask = gdk_pixmap_new (gtk_widget_get_window (GTK_WIDGET (image)), w, h, 1);

	if (gtk_widget_get_window (GTK_WIDGET (image)) == NULL)
		return;

        cr = gdk_cairo_create (pixmap);
        cr_mask = gdk_cairo_create (bitmask);

        /* setup mask */
        cairo_rectangle (cr_mask, 0, 0, w, h);
        cairo_set_operator (cr_mask, CAIRO_OPERATOR_CLEAR);
        cairo_fill (cr_mask);

        rounded_rectangle (cr_mask, 1.0, 0.5, 0.5, radius, w - 1, h - 1);
        cairo_set_operator (cr_mask, CAIRO_OPERATOR_OVER);
        cairo_set_source_rgb (cr_mask, 1, 1, 1);
        cairo_fill (cr_mask);

        color = gtk_widget_get_style (GTK_WIDGET (image))->bg [GTK_STATE_NORMAL];
        r = (float)color.red / 65535.0;
        g = (float)color.green / 65535.0;
        b = (float)color.blue / 65535.0;

        /* set up image */
        cairo_rectangle (cr, 0, 0, w, h);
        cairo_set_source_rgb (cr, r, g, b);
        cairo_fill (cr);

        rounded_rectangle (cr,
                           1.0,
                           frame_width + 0.5,
                           frame_width + 0.5,
                           radius,
                           w - frame_width * 2 - 1,
                           h - frame_width * 2 - 1);
        cairo_set_source_rgba (cr, 0.5, 0.5, 0.5, 0.3);
        cairo_fill_preserve (cr);

        surface = surface_from_pixbuf (source);
        cairo_set_source_surface (cr, surface, frame_width, frame_width);
        cairo_fill (cr);

        gtk_image_set_from_pixmap (image, pixmap, bitmask);

        cairo_surface_destroy (surface);

        g_object_unref (bitmask);
        g_object_unref (pixmap);

        cairo_destroy (cr_mask);
        cairo_destroy (cr);
}

static void
update_album_art(MetadataWidget* self){
	MetadataWidgetPrivate * priv = METADATA_WIDGET_GET_PRIVATE(self);	
	GdkPixbuf* pixbuf;
	pixbuf = gdk_pixbuf_new_from_file(priv->image_path, NULL);
	pixbuf = gdk_pixbuf_scale_simple(pixbuf,60, 60, GDK_INTERP_BILINEAR);
	g_debug("attempting to set the image with path %s", priv->image_path);
	image_set_from_pixbuf (GTK_IMAGE(priv->album_art), pixbuf);
	g_object_unref(pixbuf);	
}

// TODO refactor next 3 methods into one once the style has been 
// "signed off" by design
static void
style_artist_text(MetadataWidget* self)
{
	MetadataWidgetPrivate * priv = METADATA_WIDGET_GET_PRIVATE(self);	
	char* markup;
	markup = g_markup_printf_escaped ("<span size=\"small\">%s</span>",
	                                  gtk_label_get_text(GTK_LABEL(priv->artist_label)));
	gtk_label_set_markup (GTK_LABEL (priv->artist_label), markup);
	g_free(markup);
}

static void
style_title_text(MetadataWidget* self)
{
	MetadataWidgetPrivate * priv = METADATA_WIDGET_GET_PRIVATE(self);	

	char* markup;
	markup = g_markup_printf_escaped ("<span weight=\"bold\">%s</span>",
	                                  gtk_label_get_text(GTK_LABEL(priv->piece_label)));
	gtk_label_set_markup (GTK_LABEL (priv->piece_label), markup);
	g_free(markup);
}

static void
style_album_text(MetadataWidget* self)
{
	MetadataWidgetPrivate * priv = METADATA_WIDGET_GET_PRIVATE(self);	
	char* markup;
	markup = g_markup_printf_escaped ("<span size=\"small\">%s</span>",
	                                  gtk_label_get_text(GTK_LABEL(priv->container_label)));
	gtk_label_set_markup (GTK_LABEL (priv->container_label), markup);
	g_free(markup);
}

 /**
 * transport_new:
 * @returns: a new #MetadataWidget.
 **/
GtkWidget* 
metadata_widget_new(DbusmenuMenuitem *item)
{
	twin_item = item;
	return g_object_new(METADATA_WIDGET_TYPE, NULL);
}

