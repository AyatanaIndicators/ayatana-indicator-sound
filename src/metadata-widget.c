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
  GString*	 image_path;
	GtkWidget* artist_label;
	GtkWidget* piece_label;
	GtkWidget* container_label;	
	GdkColor   bevel_colour;
	GdkColor   eight_note_colour;
};

#define METADATA_WIDGET_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), METADATA_WIDGET_TYPE, MetadataWidgetPrivate))

/* Prototypes */
static void metadata_widget_class_init (MetadataWidgetClass *klass);
static void metadata_widget_init       (MetadataWidget *self);
static void metadata_widget_dispose    (GObject *object);
static void metadata_widget_finalize   (GObject *object);
static gboolean metadata_image_expose (GtkWidget *image, GdkEventExpose *event, gpointer user_data);

// keyevent consumers
static gboolean metadata_widget_button_press_event (GtkWidget *menuitem, 
                                  									GdkEventButton *event);
// Dbusmenuitem properties update callback
static void metadata_widget_property_update (DbusmenuMenuitem* item,
                                             gchar* property, 
                                       			 GValue* value,
                                             gpointer userdata);

static void metadata_widget_update_album_art(MetadataWidget* self);
static void metadata_widget_style_title_text(MetadataWidget* self);
static void metadata_widget_style_artist_and_album_label(MetadataWidget* self,
                                                         GtkLabel* label);

void metadata_widget_set_style(GtkWidget* button, GtkStyle* style);


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
	g_debug("MetadataWidget::metadata_widget_init");

	MetadataWidgetPrivate * priv = METADATA_WIDGET_GET_PRIVATE(self);

	GtkWidget *hbox;

	hbox = gtk_hbox_new(FALSE, 0);
	priv->hbox = hbox;

	// image
	priv->album_art = gtk_image_new();
	priv->image_path = g_string_new(dbusmenu_menuitem_property_get(twin_item, DBUSMENU_METADATA_MENUITEM_ARTURL));

	g_debug("Metadata::At startup and image path = %s", priv->image_path->str);
	
	metadata_widget_update_album_art(self);	
  g_signal_connect(priv->album_art, "expose-event", G_CALLBACK(metadata_image_expose), GTK_WIDGET(self));		
	gtk_widget_set_size_request(GTK_WIDGET(priv->album_art), 60, 60); 
	
	gtk_box_pack_start (GTK_BOX (priv->hbox), priv->album_art, FALSE, FALSE, 0);	
	
	GtkWidget* vbox = gtk_vbox_new(FALSE, 0);
	
	// artist
	GtkWidget* artist;
	artist = gtk_label_new(dbusmenu_menuitem_property_get(twin_item,
	                                                      DBUSMENU_METADATA_MENUITEM_ARTIST));
	gtk_misc_set_alignment(GTK_MISC(artist), (gfloat)0, (gfloat)0);
	gtk_misc_set_padding (GTK_MISC(artist), (gfloat)10, (gfloat)0);
	gtk_label_set_width_chars(GTK_LABEL(artist), 15);	
	gtk_label_set_ellipsize(GTK_LABEL(artist), PANGO_ELLIPSIZE_MIDDLE);	
	metadata_widget_style_artist_and_album_label(self, GTK_LABEL(artist));
	priv->artist_label = artist;
	
	// title
	GtkWidget* piece;
	piece = gtk_label_new(dbusmenu_menuitem_property_get(twin_item,
	                                                     DBUSMENU_METADATA_MENUITEM_TITLE));
	gtk_misc_set_alignment(GTK_MISC(piece), (gfloat)0, (gfloat)0);
	gtk_misc_set_padding (GTK_MISC(piece), (gfloat)10, (gfloat)0);
	gtk_label_set_width_chars(GTK_LABEL(piece), 15);
	gtk_label_set_ellipsize(GTK_LABEL(piece), PANGO_ELLIPSIZE_MIDDLE);
	priv->piece_label =  piece;
	metadata_widget_style_title_text(self);

	// container
	GtkWidget* container;
	container = gtk_label_new(dbusmenu_menuitem_property_get(twin_item,
	                                                         DBUSMENU_METADATA_MENUITEM_ALBUM));
	gtk_misc_set_alignment(GTK_MISC(container), (gfloat)0, (gfloat)0);
	gtk_misc_set_padding (GTK_MISC(container), (gfloat)10, (gfloat)0);	
	gtk_label_set_width_chars(GTK_LABEL(container), 15);		
	gtk_label_set_ellipsize(GTK_LABEL(container), PANGO_ELLIPSIZE_MIDDLE);	
	metadata_widget_style_artist_and_album_label(self, GTK_LABEL(container));
	priv->container_label = container;

	gtk_box_pack_start (GTK_BOX (vbox), priv->piece_label, FALSE, FALSE, 0);	
	gtk_box_pack_start (GTK_BOX (vbox), priv->artist_label, FALSE, FALSE, 0);	
	gtk_box_pack_start (GTK_BOX (vbox), priv->container_label, FALSE, FALSE, 0);	
	
	gtk_box_pack_start (GTK_BOX (priv->hbox), vbox, FALSE, FALSE, 0);	

	g_signal_connect(G_OBJECT(twin_item), "property-changed", 
	                 G_CALLBACK(metadata_widget_property_update), self);
	gtk_widget_show_all (priv->hbox);

  g_signal_connect(self, "style-set", G_CALLBACK(metadata_widget_set_style), GTK_WIDGET(self));		
	
	gtk_widget_set_size_request(GTK_WIDGET(self), 200, 60); 
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
 * empty album art image (and soon rounded rectangles on the album art)
 */
static gboolean
metadata_image_expose (GtkWidget *metadata, GdkEventExpose *event, gpointer user_data)
{
	g_return_val_if_fail(IS_METADATA_WIDGET(user_data), FALSE);
	MetadataWidget* widget = METADATA_WIDGET(user_data);
	MetadataWidgetPrivate * priv = METADATA_WIDGET_GET_PRIVATE(widget);	

	if(priv->image_path->len > 0){
			return FALSE;	
	}
	
	cairo_t *cr;	
	cr = gdk_cairo_create (metadata->window);
	GtkAllocation alloc;
	gtk_widget_get_allocation (metadata, &alloc);
		
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

	cairo_set_source_rgba (cr, 123.0f / 255.0f, 123.0f / 255.0f, 120.0f / 255.0f, .8f);		
	cairo_set_line_width (cr, 2.0);
	
	cairo_stroke (cr);						

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

	cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.8);
	pango_cairo_update_layout(cr, layout);
	cairo_move_to (cr, alloc.x + alloc.width/6, alloc.y);	
	pango_cairo_show_layout(cr, layout);

	g_object_unref(layout);	

	cairo_destroy (cr);	
	
	return TRUE;
}

/* Suppress/consume keyevents */
static gboolean
metadata_widget_button_press_event (GtkWidget *menuitem, 
                                  GdkEventButton *event)
{
	GtkClipboard* board = gtk_clipboard_get (GDK_NONE);	
	gchar* title = g_strdup(dbusmenu_menuitem_property_get(twin_item,
	                                              DBUSMENU_METADATA_MENUITEM_TITLE));
	gchar* artist = g_strdup(dbusmenu_menuitem_property_get(twin_item,
	                                              DBUSMENU_METADATA_MENUITEM_ARTIST));
	gchar* album = g_strdup(dbusmenu_menuitem_property_get(twin_item,
	                                              DBUSMENU_METADATA_MENUITEM_ALBUM));
	gchar* contents = g_strdup_printf("artist: %s \ntitle: %s \nalbum: %s", artist, title, album);
	g_debug("contents to be copied will be : %s", contents);	
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
		metadata_widget_style_artist_and_album_label(mitem, GTK_LABEL(priv->artist_label));
	}
	else if(g_ascii_strcasecmp(DBUSMENU_METADATA_MENUITEM_TITLE, property) == 0){  
		gtk_label_set_text(GTK_LABEL(priv->piece_label), g_value_get_string(value));		
		metadata_widget_style_title_text(mitem);
	}	
	else if(g_ascii_strcasecmp(DBUSMENU_METADATA_MENUITEM_ALBUM, property) == 0){  
		gtk_label_set_text(GTK_LABEL(priv->container_label), g_value_get_string(value));
		metadata_widget_style_artist_and_album_label(mitem, GTK_LABEL(priv->container_label));
	}	
	else if(g_ascii_strcasecmp(DBUSMENU_METADATA_MENUITEM_ARTURL, property) == 0){
		g_string_erase(priv->image_path, 0, -1);
		g_string_overwrite(priv->image_path, 0, g_value_dup_string(value)); 
		metadata_widget_update_album_art(mitem);			
	}		
}

static void
metadata_widget_update_album_art(MetadataWidget* self){
	MetadataWidgetPrivate * priv = METADATA_WIDGET_GET_PRIVATE(self);	
	GdkPixbuf* pixbuf;
	pixbuf = gdk_pixbuf_new_from_file(priv->image_path->str, NULL);
	g_debug("metadata_widget_update_album_art -> pixbuf from %s",
	        priv->image_path->str); 
  pixbuf = gdk_pixbuf_scale_simple(pixbuf,60, 60, GDK_INTERP_BILINEAR);
	gtk_image_set_from_pixbuf(GTK_IMAGE(priv->album_art), pixbuf);
	g_object_unref(pixbuf);	
}

// TODO refactor next 3 methods into one once the style has been 
static void
metadata_widget_style_artist_and_album_label(MetadataWidget* self, GtkLabel* label)
{
	char* markup;
	markup = g_markup_printf_escaped ("<span size=\"small\">%s</span>",
	                                  gtk_label_get_text(GTK_LABEL(label)));
	gtk_label_set_markup (GTK_LABEL (label), markup);
	g_free(markup);	
}

static void
metadata_widget_style_title_text(MetadataWidget* self)
{
	MetadataWidgetPrivate * priv = METADATA_WIDGET_GET_PRIVATE(self);	

	char* markup;
	markup = g_markup_printf_escaped ("<span weight=\"bold\" size=\"small\">%s</span>",
	                                  gtk_label_get_text(GTK_LABEL(priv->piece_label)));
	gtk_label_set_markup (GTK_LABEL (priv->piece_label), markup);
	g_free(markup);
}

void
metadata_widget_set_style(GtkWidget* metadata, GtkStyle* style)
{
	g_return_if_fail(IS_METADATA_WIDGET(metadata));
	MetadataWidget* widg = METADATA_WIDGET(metadata);
	MetadataWidgetPrivate * priv = METADATA_WIDGET_GET_PRIVATE(widg);	
	if(style == NULL){
		g_warning("metadata_widget_set_style -> style is NULL!");
		return;
	}
	else{
		g_debug("metadata_widget: about to set the style colours");
		priv->eight_note_colour = style->fg[GTK_STATE_NORMAL];
		priv->bevel_colour = style->bg[GTK_STATE_NORMAL];		
	}
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

