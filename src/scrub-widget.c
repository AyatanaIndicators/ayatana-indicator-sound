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
#include "scrub-widget.h"
#include "common-defs.h"
#include <libido/idoscalemenuitem.h>

typedef struct _ScrubWidgetPrivate ScrubWidgetPrivate;

struct _ScrubWidgetPrivate
{
	DbusmenuMenuitem* twin_item;	
	GtkWidget* ido_scrub_bar;
	GtkStyle* style;	
};

#define SCRUB_WIDGET_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), SCRUB_WIDGET_TYPE, ScrubWidgetPrivate))

/* Prototypes */
static void scrub_widget_class_init (ScrubWidgetClass *klass);
static void scrub_widget_init       (ScrubWidget *self);
static void scrub_widget_dispose    (GObject *object);
static void scrub_widget_finalize   (GObject *object);
static void scrub_widget_property_update( DbusmenuMenuitem* item, gchar* property, 
                                       	  GValue* value, gpointer userdata);
static void scrub_widget_set_twin_item(	ScrubWidget* self,
                           							DbusmenuMenuitem* twin_item);
static void scrub_widget_parent_changed ( GtkWidget *widget, gpointer	user_data);
static gchar* scrub_widget_format_time(gint time);
static gboolean scrub_widget_change_value_cb (GtkRange     *range,
                                 							GtkScrollType scroll,
                                 							gdouble       value,
                                 							gpointer      user_data);


G_DEFINE_TYPE (ScrubWidget, scrub_widget, G_TYPE_OBJECT);

static void
scrub_widget_class_init (ScrubWidgetClass *klass)
{
	GObjectClass 			*gobject_class = G_OBJECT_CLASS (klass);
	
	g_type_class_add_private (klass, sizeof (ScrubWidgetPrivate));

	gobject_class->dispose = scrub_widget_dispose;
	gobject_class->finalize = scrub_widget_finalize;
}


static void
scrub_widget_init (ScrubWidget *self)
{
	g_debug("ScrubWidget::scrub_widget_init");
	ScrubWidgetPrivate * priv = SCRUB_WIDGET_GET_PRIVATE(self);

  priv->ido_scrub_bar = ido_scale_menu_item_new_with_range ("Scrub", 0, 0, 100, 1);
	ido_scale_menu_item_set_style (IDO_SCALE_MENU_ITEM(priv->ido_scrub_bar), IDO_SCALE_MENU_ITEM_STYLE_LABEL);	
	ido_scale_menu_item_set_primary_label(IDO_SCALE_MENU_ITEM(priv->ido_scrub_bar), "00:00"); 	
	ido_scale_menu_item_set_secondary_label(IDO_SCALE_MENU_ITEM(priv->ido_scrub_bar), "05:35"); 	
	g_object_set(priv->ido_scrub_bar, "reverse-scroll-events", TRUE, NULL);

  //g_signal_connect (priv->ido_scrub_bar,
  //                  "notify::parent", G_CALLBACK (scrub_widget_parent_changed),
  //                  NULL);
	
  // register slider changes listening on the range
  GtkWidget* scrub_widget = ido_scale_menu_item_get_scale((IdoScaleMenuItem*)priv->ido_scrub_bar);	
  g_signal_connect(scrub_widget, "change-value", G_CALLBACK(scrub_widget_change_value_cb), self);	
}

static void
scrub_widget_dispose (GObject *object)
{
	G_OBJECT_CLASS (scrub_widget_parent_class)->dispose (object);
}

static void
scrub_widget_finalize (GObject *object)
{
	G_OBJECT_CLASS (scrub_widget_parent_class)->finalize (object);
}

static void 
scrub_widget_property_update(DbusmenuMenuitem* item, gchar* property, 
                                       GValue* value, gpointer userdata)
{
	g_return_if_fail (IS_SCRUB_WIDGET (userdata));	
	ScrubWidget* mitem = SCRUB_WIDGET(userdata);
	ScrubWidgetPrivate * priv = SCRUB_WIDGET_GET_PRIVATE(mitem);
	
	if(g_ascii_strcasecmp(DBUSMENU_SCRUB_MENUITEM_DURATION, property) == 0){
		ido_scale_menu_item_set_secondary_label(IDO_SCALE_MENU_ITEM(priv->ido_scrub_bar),
		                                      scrub_widget_format_time(g_value_get_int(value))); 			
	}
	else if(g_ascii_strcasecmp(DBUSMENU_SCRUB_MENUITEM_POSITION, property) == 0){
		ido_scale_menu_item_set_primary_label(IDO_SCALE_MENU_ITEM(priv->ido_scrub_bar),
		                                      scrub_widget_format_time(g_value_get_int(value))); 					
	}	
}

/*static void
scrub_widget_parent_changed (GtkWidget *widget,
                       gpointer   user_data)
{
  gtk_widget_set_size_request (widget, 200, -1);
  g_debug("slider parent changed");
}*/

static void
scrub_widget_set_twin_item(ScrubWidget* self,
                           DbusmenuMenuitem* twin_item)
{
	ScrubWidgetPrivate * priv = SCRUB_WIDGET_GET_PRIVATE(self);
	priv->twin_item = twin_item;

	g_signal_connect(G_OBJECT(twin_item), "property-changed", 
	                 G_CALLBACK(scrub_widget_property_update), self);
}

static gboolean
scrub_widget_change_value_cb (GtkRange     *range,
                 							GtkScrollType scroll,
                 							gdouble       new_value,
                 							gpointer      user_data)
{
	g_return_val_if_fail (IS_SCRUB_WIDGET (user_data), FALSE);
	ScrubWidget* mitem = SCRUB_WIDGET(user_data);
	ScrubWidgetPrivate * priv = SCRUB_WIDGET_GET_PRIVATE(mitem);

	GValue value = {0};
  g_value_init(&value, G_TYPE_DOUBLE);
	gdouble clamped = CLAMP(new_value, 0, 100);
  g_value_set_double(&value, clamped);
  g_debug("scrub-widget-change-value callback - = %f", clamped);
  dbusmenu_menuitem_handle_event (priv->twin_item, "scrubbing", &value, 0);
	return FALSE;
}

GtkWidget*
scrub_widget_get_ido_bar(ScrubWidget* self)
{
	ScrubWidgetPrivate * priv = SCRUB_WIDGET_GET_PRIVATE(self);
	return priv->ido_scrub_bar;	
}

static gchar*
scrub_widget_format_time(gint time)
{
	// Assuming its in seconds for now ...
	gint minutes = time/60;
	gint seconds = time % 60;
	gchar* prefix="0";
	if(minutes > 9)
		prefix="";
	return g_strdup_printf("%s%i:%i", prefix, minutes, seconds);	
}
                           
 /**
 * scrub_widget_new:
 * @returns: a new #ScrubWidget.
 **/
GtkWidget* 
scrub_widget_new(DbusmenuMenuitem *item)
{
	GtkWidget* widget = g_object_new(SCRUB_WIDGET_TYPE, NULL);
	scrub_widget_set_twin_item((ScrubWidget*)widget, item);
	return widget;
}

