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
	GdkColor   bevel_colour;
	GdkColor   eight_note_colour;
};

#define METADATA_WIDGET_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), METADATA_WIDGET_TYPE, MetadataWidgetPrivate))

/* Prototypes */
static void metadata_widget_class_init (MetadataWidgetClass *klass);
static void metadata_widget_init       (MetadataWidget *self);
static void metadata_widget_dispose    (GObject *object);
static void metadata_widget_finalize   (GObject *object);

// keyevent consumers
static gboolean metadata_widget_button_press_event (GtkWidget *menuitem, 
                                  									GdkEventButton *event);
// Dbusmenuitem properties update callback
static void metadata_widget_property_update(DbusmenuMenuitem* item, gchar* property, 
                                       GValue* value, gpointer userdata);

static void metadata_widget_update_album_art(MetadataWidget* self);
static void metadata_widget_style_artist_text(MetadataWidget* self);
static void metadata_widget_style_title_text(MetadataWidget* self);
static void metadata_widget_style_album_text(MetadataWidget* self);
static void metadata_widget_draw_album_art_placeholder(MetadataWidget* self);

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
	priv->image_path = g_strdup(dbusmenu_menuitem_property_get(twin_item, DBUSMENU_METADATA_MENUITEM_ARTURL));
	if(priv->image_path != NULL){
		metadata_widget_update_album_art(self);	
	}
	else{
		metadata_widget_draw_album_art_placeholder(self);			
	}

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
	metadata_widget_style_artist_text(self);
	
	// title
	GtkWidget* piece;
	piece = gtk_label_new(dbusmenu_menuitem_property_get(twin_item,
	                                                     DBUSMENU_METADATA_MENUITEM_TITLE));
	gtk_misc_set_alignment(GTK_MISC(piece), (gfloat)0, (gfloat)0);
	gtk_label_set_width_chars(GTK_LABEL(piece), 12);
	gtk_label_set_ellipsize(GTK_LABEL(piece), PANGO_ELLIPSIZE_MIDDLE);
	priv->piece_label =  piece;
	metadata_widget_style_title_text(self);

	// container
	GtkWidget* container;
	container = gtk_label_new(dbusmenu_menuitem_property_get(twin_item,
	                                                         DBUSMENU_METADATA_MENUITEM_ALBUM));
	gtk_misc_set_alignment(GTK_MISC(container), (gfloat)0, (gfloat)0);
	gtk_label_set_width_chars(GTK_LABEL(container), 15);		
	gtk_label_set_ellipsize(GTK_LABEL(container), PANGO_ELLIPSIZE_MIDDLE);	
	priv->container_label = container;
	metadata_widget_style_album_text(self);

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

/* Suppress/consume keyevents */
static gboolean
metadata_widget_button_press_event (GtkWidget *menuitem, 
                                  GdkEventButton *event)
{
	GtkClipboard* board = gtk_clipboard_get (GDK_NONE);	
	gchar* title = dbusmenu_menuitem_property_get(twin_item,
	                                              DBUSMENU_METADATA_MENUITEM_TITLE);
	gchar* artist = dbusmenu_menuitem_property_get(twin_item,
	                                              DBUSMENU_METADATA_MENUITEM_ARTIST);
	gchar* album = dbusmenu_menuitem_property_get(twin_item,
	                                              DBUSMENU_METADATA_MENUITEM_ALBUM);
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
		metadata_widget_style_artist_text(mitem);
	}
	else if(g_ascii_strcasecmp(DBUSMENU_METADATA_MENUITEM_TITLE, property) == 0){  
		gtk_label_set_text(GTK_LABEL(priv->piece_label), g_value_get_string(value));		
		metadata_widget_style_title_text(mitem);
	}	
	else if(g_ascii_strcasecmp(DBUSMENU_METADATA_MENUITEM_ALBUM, property) == 0){  
		gtk_label_set_text(GTK_LABEL(priv->container_label), g_value_get_string(value));
		metadata_widget_style_album_text(mitem);	
	}	
	else if(g_ascii_strcasecmp(DBUSMENU_METADATA_MENUITEM_ARTURL, property) == 0){
		if(priv->image_path != NULL){
			g_free(priv->image_path);
		}
		priv->image_path = g_value_dup_string(value);
		if(priv->image_path != NULL){
			metadata_widget_update_album_art(mitem);
		}
		else{
			metadata_widget_draw_album_art_placeholder(mitem);
		}
	}		
}

static void metadata_widget_draw_album_art_placeholder(MetadataWidget* self)
{
	/*MetadataWidgetPrivate * priv = METADATA_WIDGET_GET_PRIVATE(self);			
	cairo_t *cr;	
	cr = gdk_cairo_create (GTK_WIDGET(self)->window);
	PangoLayout *layout;
	PangoFontDescription *desc;
	layout = pango_cairo_create_layout(cr);

	GString* string = g_string_new("");
	gssize size = -1;
	gunichar code = g_utf8_get_char_validated("0x266B", size);	
	g_string_append_unichar (string, code);

	pango_layout_set_text(layout, string->str, -1);
	desc = pango_font_description_from_string("Sans Bold 12");
	pango_layout_set_font_description(layout, desc);
	pango_font_description_free(desc);

	cairo_set_source_rgb(cr, 0.0, 0.0, 1.0);
	pango_cairo_update_layout(cr, layout);
	pango_cairo_show_layout(cr, layout);

	g_object_unref(layout);	
	cairo_destroy (cr);	
	*/
}

static void
metadata_widget_update_album_art(MetadataWidget* self){
	MetadataWidgetPrivate * priv = METADATA_WIDGET_GET_PRIVATE(self);	
	GdkPixbuf* pixbuf;
	pixbuf = gdk_pixbuf_new_from_file(priv->image_path, NULL);
  pixbuf = gdk_pixbuf_scale_simple(pixbuf,60, 60, GDK_INTERP_BILINEAR);
	g_debug("attempting to set the image with path %s", priv->image_path);
	gtk_image_set_from_pixbuf(GTK_IMAGE(priv->album_art), pixbuf);
	g_object_unref(pixbuf);	
}

// TODO refactor next 3 methods into one once the style has been 
// "signed off" by design
static void
metadata_widget_style_artist_text(MetadataWidget* self)
{
	MetadataWidgetPrivate * priv = METADATA_WIDGET_GET_PRIVATE(self);	
	char* markup;
	markup = g_markup_printf_escaped ("<span size=\"small\">%s</span>",
	                                  gtk_label_get_text(GTK_LABEL(priv->artist_label)));
	gtk_label_set_markup (GTK_LABEL (priv->artist_label), markup);
	g_free(markup);
}

static void
metadata_widget_style_title_text(MetadataWidget* self)
{
	MetadataWidgetPrivate * priv = METADATA_WIDGET_GET_PRIVATE(self);	

	char* markup;
	markup = g_markup_printf_escaped ("<span weight=\"bold\">%s</span>",
	                                  gtk_label_get_text(GTK_LABEL(priv->piece_label)));
	gtk_label_set_markup (GTK_LABEL (priv->piece_label), markup);
	g_free(markup);
}

static void
metadata_widget_style_album_text(MetadataWidget* self)
{
	MetadataWidgetPrivate * priv = METADATA_WIDGET_GET_PRIVATE(self);	
	char* markup;
	markup = g_markup_printf_escaped ("<span size=\"small\">%s</span>",
	                                  gtk_label_get_text(GTK_LABEL(priv->container_label)));
	gtk_label_set_markup (GTK_LABEL (priv->container_label), markup);
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

