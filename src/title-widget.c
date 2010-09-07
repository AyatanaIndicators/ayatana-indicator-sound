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
#include "title-widget.h"
#include "common-defs.h"
#include <gtk/gtk.h>
#include <libindicator/indicator-image-helper.h>


typedef struct _TitleWidgetPrivate TitleWidgetPrivate;

struct _TitleWidgetPrivate
{
	//GtkWidget* hbox;
	//GtkWidget* name;
	GtkWidget* image_item;	
	DbusmenuMenuitem* twin_item;	
};

#define TITLE_WIDGET_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TITLE_WIDGET_TYPE, TitleWidgetPrivate))

/* Prototypes */
static void title_widget_class_init (TitleWidgetClass *klass);
static void title_widget_init       (TitleWidget *self);
static void title_widget_dispose    (GObject *object);
static void title_widget_finalize   (GObject *object);

// keyevent consumers
static gboolean title_widget_button_press_event (GtkWidget *menuitem, 
                                  							 GdkEventButton *event);

// Dbusmenuitem properties update callback
static void title_widget_property_update(DbusmenuMenuitem* item, gchar* property, 
                                       GValue* value, gpointer userdata);
static void title_widget_set_twin_item(	TitleWidget* self,
                           							DbusmenuMenuitem* twin_item);
//static void title_widget_style_name_text(TitleWidget* self);

static gboolean title_widget_triangle_draw_cb (GtkWidget *widget,
                                               GdkEventExpose *event,
                                               gpointer data);

G_DEFINE_TYPE (TitleWidget, title_widget, GTK_TYPE_IMAGE_MENU_ITEM);



static void
title_widget_class_init (TitleWidgetClass *klass)
{
	GObjectClass 			*gobject_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass    *widget_class = GTK_WIDGET_CLASS (klass);

	widget_class->button_press_event = title_widget_button_press_event;
	
	g_type_class_add_private (klass, sizeof (TitleWidgetPrivate));

	gobject_class->dispose = title_widget_dispose;
	gobject_class->finalize = title_widget_finalize;
}

static void
title_widget_init (TitleWidget *self)
{
	g_debug("TitleWidget::title_widget_init");

	TitleWidgetPrivate * priv = TITLE_WIDGET_GET_PRIVATE(self);

	//GtkWidget *hbox;
  
	//hbox = gtk_hbox_new(FALSE, 0);
	//priv->hbox = hbox;

	// Add image to the 'gutter'
	gtk_image_menu_item_set_always_show_image(GTK_IMAGE_MENU_ITEM(self), TRUE);	
	gtk_image_menu_item_set_use_stock(GTK_IMAGE_MENU_ITEM(self), FALSE);
	
	gint padding = 4;
	gtk_widget_style_get(GTK_WIDGET(self), "horizontal-padding", &padding, NULL);

	gint width, height;
	gtk_icon_size_lookup(GTK_ICON_SIZE_MENU, &width, &height);

	GtkImage * image = indicator_image_helper("sound_icon");
	GdkPixbuf* buf = gtk_image_get_pixbuf (image);
	
	//GtkWidget * icon = gtk_image_new_from_icon_name("sound_icon", GTK_ICON_SIZE_MENU);
	GtkWidget * icon = gtk_image_new_from_file("/usr/share/icons/ubuntu-mono-dark/status/16/sound_icon.png");
	
	GtkAllocation new_alloc;
	new_alloc.width = 16;
	new_alloc.height = 16;
	new_alloc.x = 16;
	new_alloc.y = 16;
	
	gtk_widget_set_allocation(icon, &new_alloc);
	
	gtk_widget_set_size_request(icon, width
															+ 5 /* ref triangle is 5x9 pixels */
															+ 2 /* padding */,
															height);
	gtk_misc_set_alignment(GTK_MISC(icon), 1.0 /* right aligned */, 0.5);
	
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(self), GTK_WIDGET(icon));
	
	//gtk_widget_show_all(icon);
	
	// DEBUG
	g_debug("title widget init - Is there a pixbuf from image loaded with helper : %i", GDK_IS_PIXBUF(buf));

	g_debug("title widget init - icon pixel size = %i", gtk_image_get_pixel_size (GTK_IMAGE(icon)));
	g_debug("title widget init - image pixel size = %i", gtk_image_get_pixel_size  (image));

	g_debug("title widget init - height and weight = %i and %i", height, width);
	
	GtkImageType type;
	type = gtk_image_get_storage_type(GTK_IMAGE(icon));
	g_debug("title widget init - gtk_image_storage_type on widget = %i", type);
	type = gtk_image_get_storage_type(image);
	g_debug("title widget init - gtk_image_storage_type on image = %i", type);	

	GtkWidget* returned_image = gtk_image_menu_item_get_image(GTK_IMAGE_MENU_ITEM(self));
	g_debug("title widget init - returned image is not null %i", GTK_IS_IMAGE(returned_image));

	gboolean* use_stock;
  use_stock = g_new0(gboolean, 1);
	gboolean* show_image;
  show_image = g_new0(gboolean, 1);

	g_object_get(GTK_WIDGET(self), "use-stock", use_stock, NULL );
	g_object_get(GTK_WIDGET(self), "always-show-image", show_image, NULL);
	
	GtkAllocation alloc;
	gtk_widget_get_allocation(icon, &alloc);

	g_debug("title widget init - alloc for icon: width : %i, height : %i, x : %i and y : %i",
	        alloc.width,
	        alloc.height,
	        alloc.x,
	        alloc.y); 
	
	g_debug("title widget init : use-stock = %i and show image = %i", *use_stock, *show_image);
	g_free(use_stock);
	g_free(show_image);	
}

static void
title_widget_dispose (GObject *object)
{
	G_OBJECT_CLASS (title_widget_parent_class)->dispose (object);
}

static void
title_widget_finalize (GObject *object)
{
	G_OBJECT_CLASS (title_widget_parent_class)->finalize (object);
}

/* Suppress/consume keyevents */
static gboolean
title_widget_button_press_event (GtkWidget *menuitem, 
                                  GdkEventButton *event)
{
	g_debug("TitleWidget::menu_press_event");
	TitleWidgetPrivate * priv = TITLE_WIDGET_GET_PRIVATE(menuitem);
	
	GValue value = {0};
  g_value_init(&value, G_TYPE_BOOLEAN);

	g_value_set_boolean(&value, TRUE);	
	dbusmenu_menuitem_handle_event (priv->twin_item, "Title menu event", &value, 0);
	
	return FALSE;
}

static void 
title_widget_property_update(DbusmenuMenuitem* item, gchar* property, 
                                       GValue* value, gpointer userdata)
{
	g_return_if_fail (IS_TITLE_WIDGET (userdata));	
	TitleWidget* mitem = TITLE_WIDGET(userdata);
	TitleWidgetPrivate * priv = TITLE_WIDGET_GET_PRIVATE(mitem);
	
	if(g_ascii_strcasecmp(DBUSMENU_TITLE_MENUITEM_NAME, property) == 0){  
  	gtk_menu_item_set_label (GTK_MENU_ITEM(mitem),
                             g_value_get_string(value));    
		//gtk_label_set_text(GTK_LABEL(priv->name), g_value_get_string(value));
		//title_widget_style_name_text(mitem);
	}
}

static void
title_widget_set_twin_item(TitleWidget* self,
                           DbusmenuMenuitem* twin_item)
{
	TitleWidgetPrivate * priv = TITLE_WIDGET_GET_PRIVATE(self);
	priv->twin_item = twin_item;
	g_signal_connect(G_OBJECT(twin_item), "property-changed", 
	                 G_CALLBACK(title_widget_property_update), self);	
	g_signal_connect_after(G_OBJECT (self),
	                       "expose_event", G_CALLBACK (title_widget_triangle_draw_cb), twin_item);
	
	// Add the application name
	//priv->name = gtk_label_new(dbusmenu_menuitem_property_get(priv->twin_item, 
	//                                                          DBUSMENU_TITLE_MENUITEM_NAME));
	//gtk_misc_set_padding(GTK_MISC(priv->name), 0, 0);
	//gtk_box_pack_start (GTK_BOX (priv->hbox), priv->name, FALSE, FALSE, 0);		

	//title_widget_style_name_text(self);
	gtk_menu_item_set_label (GTK_MENU_ITEM(self), 
                           dbusmenu_menuitem_property_get(priv->twin_item,
                                                          DBUSMENU_TITLE_MENUITEM_NAME));
  
  //gtk_container_add (GTK_CONTAINER (self), GTK_WIDGET(priv->hbox));	
	gtk_widget_show_all (GTK_WIDGET(self));	
}
                           
/*static void
title_widget_style_name_text(TitleWidget* self)
{
	TitleWidgetPrivate * priv = TITLE_WIDGET_GET_PRIVATE(self);

	char* markup;
	markup = g_markup_printf_escaped ("<span size=\"medium\">%s</span>",
	                                  gtk_label_get_text(GTK_LABEL(priv->name)));
	gtk_label_set_markup (GTK_LABEL (priv->name), markup);
	g_free(markup);
}*/

static gboolean
title_widget_triangle_draw_cb (GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
	GtkStyle *style;
	cairo_t *cr;
	int x, y, arrow_width, arrow_height;
	
	if (!GTK_IS_WIDGET (widget)) return FALSE;
	if (!DBUSMENU_IS_MENUITEM (data)) return FALSE;

	/* render the triangle indicator only if the application is running */
	if (! dbusmenu_menuitem_property_get_bool (DBUSMENU_MENUITEM(data),
	                                           DBUSMENU_TITLE_MENUITEM_RUNNING)){		
		return FALSE;
	}
	
	/* get style */
	style = gtk_widget_get_style (widget);

	/* set arrow position / dimensions */
	arrow_width = 5; /* the pixel-based reference triangle is 5x9 */
	arrow_height = 9;
	x = widget->allocation.x;
	y = widget->allocation.y + widget->allocation.height/2.0 - (double)arrow_height/2.0;

	/* initialize cairo drawing area */
	cr = (cairo_t*) gdk_cairo_create (widget->window);

	/* set line width */	
	cairo_set_line_width (cr, 1.0);

	/* cairo drawing code */
	cairo_move_to (cr, x, y);
	cairo_line_to (cr, x, y + arrow_height);
	cairo_line_to (cr, x + arrow_width, y + (double)arrow_height/2.0);
	cairo_close_path (cr);
	cairo_set_source_rgb (cr, style->fg[gtk_widget_get_state(widget)].red/65535.0,
	                          style->fg[gtk_widget_get_state(widget)].green/65535.0,
	                          style->fg[gtk_widget_get_state(widget)].blue/65535.0);
	cairo_fill (cr);

	/* remember to destroy cairo context to avoid leaks */
	cairo_destroy (cr);

	return FALSE;
}

 /**
 * transport_new:
 * @returns: a new #TitleWidget.
 **/
GtkWidget* 
title_widget_new(DbusmenuMenuitem *item)
{
	GtkWidget* widget = g_object_new(TITLE_WIDGET_TYPE, NULL);
	title_widget_set_twin_item((TitleWidget*)widget, item);
	return widget;
}

