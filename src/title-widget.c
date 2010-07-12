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
	GtkWidget* hbox;
	GtkWidget* name;
	GtkWidget* player_icon;	
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
static gboolean title_widget_button_release_event (GtkWidget *menuitem, 
                                  								 GdkEventButton *event);
static gboolean title_widget_expose_event(GtkWidget* widget, 
                                          GdkEventExpose* event);

// Dbusmenuitem properties update callback
static void title_widget_property_update(DbusmenuMenuitem* item, gchar* property, 
                                       GValue* value, gpointer userdata);
static void title_widget_set_twin_item(	TitleWidget* self,
                           							DbusmenuMenuitem* twin_item);
static void title_widget_style_name_text(TitleWidget* self);

G_DEFINE_TYPE (TitleWidget, title_widget, GTK_TYPE_MENU_ITEM);



static void
title_widget_class_init (TitleWidgetClass *klass)
{
	GObjectClass 			*gobject_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass    *widget_class = GTK_WIDGET_CLASS (klass);

	widget_class->button_press_event = title_widget_button_press_event;
  widget_class->button_release_event = title_widget_button_release_event;
	widget_class->expose_event = title_widget_expose_event;
	
	g_type_class_add_private (klass, sizeof (TitleWidgetPrivate));

	gobject_class->dispose = title_widget_dispose;
	gobject_class->finalize = title_widget_finalize;

}

static void
title_widget_init (TitleWidget *self)
{
	g_debug("TitleWidget::title_widget_init");

	TitleWidgetPrivate * priv = TITLE_WIDGET_GET_PRIVATE(self);

	GtkWidget *hbox;

	hbox = gtk_hbox_new(FALSE, 0);
	priv->hbox = hbox;

	priv->player_icon = indicator_image_helper("sound_icon");
	gtk_box_pack_start(GTK_BOX (priv->hbox), priv->player_icon, FALSE, FALSE, 0);		
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
	
	return TRUE;
}

static gboolean
title_widget_button_release_event (GtkWidget *menuitem, 
                                  GdkEventButton *event)
{
	g_debug("TitleWidget::menu_release_event");
	return TRUE;
}

static gboolean
title_widget_expose_event(GtkWidget* widget, GdkEventExpose* event)
{
	TitleWidgetPrivate * priv = TITLE_WIDGET_GET_PRIVATE(widget);
		
	gtk_container_propagate_expose(GTK_CONTAINER(widget), priv->hbox, event); 
	return TRUE;
}

static void 
title_widget_property_update(DbusmenuMenuitem* item, gchar* property, 
                                       GValue* value, gpointer userdata)
{
	g_return_if_fail (IS_TITLE_WIDGET (userdata));	
	TitleWidget* mitem = TITLE_WIDGET(userdata);
	TitleWidgetPrivate * priv = TITLE_WIDGET_GET_PRIVATE(mitem);
	
	if(g_ascii_strcasecmp(DBUSMENU_TITLE_MENUITEM_TEXT_NAME, property) == 0){  
		gtk_label_set_text(GTK_LABEL(priv->name), g_value_get_string(value));
		title_widget_style_name_text(mitem);
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
	priv->name = gtk_label_new(dbusmenu_menuitem_property_get(priv->twin_item, 
	                                                          DBUSMENU_TITLE_MENUITEM_TEXT_NAME));
	gtk_misc_set_padding(GTK_MISC(priv->name), 10, 0);
	gtk_box_pack_start (GTK_BOX (priv->hbox), priv->name, FALSE, FALSE, 0);		

	title_widget_style_name_text(self);
	
	gtk_widget_show_all (priv->hbox);
  gtk_container_add (GTK_CONTAINER (self), priv->hbox);	
}
                           
static void
title_widget_style_name_text(TitleWidget* self)
{
	TitleWidgetPrivate * priv = TITLE_WIDGET_GET_PRIVATE(self);

	char* markup;
	markup = g_markup_printf_escaped ("<span weight=\"bold\">%s</span>",
	                                  gtk_label_get_text(GTK_LABEL(priv->name)));
	gtk_label_set_markup (GTK_LABEL (priv->name), markup);
	g_free(markup);
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

