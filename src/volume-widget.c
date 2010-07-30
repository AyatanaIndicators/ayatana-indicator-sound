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
#include "volume-widget.h"
#include "common-defs.h"
#include <libido/idoscalemenuitem.h>

typedef struct _VolumeWidgetPrivate VolumeWidgetPrivate;

struct _VolumeWidgetPrivate
{
	DbusmenuMenuitem* twin_item;	
	GtkWidget* ido_volume_slider;
	gboolean grabbed;
};

#define VOLUME_WIDGET_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), VOLUME_WIDGET_TYPE, VolumeWidgetPrivate))

/* Prototypes */
static void volume_widget_class_init (VolumeWidgetClass *klass);
static void volume_widget_init       (VolumeWidget *self);
static void volume_widget_dispose    (GObject *object);
static void volume_widget_finalize   (GObject *object);
static void volume_widget_set_twin_item(	VolumeWidget* self,
                           							DbusmenuMenuitem* twin_item);
static void volume_widget_set_ido_position(VolumeWidget* self,
                                          gint position,
                                          gint duration);
//callbacks
static void volume_widget_property_update( DbusmenuMenuitem* item, gchar* property, 
                                       	  GValue* value, gpointer userdata);
static gboolean volume_widget_change_value_cb (GtkRange     *range,
                                 							GtkScrollType scroll,
                                 							gdouble       value,
                                 							gpointer      user_data);
static void volume_widget_slider_grabbed(GtkWidget *widget, gpointer user_data);
static void volume_widget_slider_released(GtkWidget *widget, gpointer user_data);
static void volume_widget_parent_changed (GtkWidget *widget, gpointer user_data);

G_DEFINE_TYPE (VolumeWidget, volume_widget, G_TYPE_OBJECT);

static void
volume_widget_class_init (VolumeWidgetClass *klass)
{
	GObjectClass 			*gobject_class = G_OBJECT_CLASS (klass);
	
	g_type_class_add_private (klass, sizeof (VolumeWidgetPrivate));

	gobject_class->dispose = volume_widget_dispose;
	gobject_class->finalize = volume_widget_finalize;
}

static void
volume_widget_init (VolumeWidget *self)
{
	g_debug("VolumeWidget::volume_widget_init");
	VolumeWidgetPrivate * priv = VOLUME_WIDGET_GET_PRIVATE(self);

  priv->ido_volume_slider = ido_scale_menu_item_new_with_range ("VOLUME", IDO_RANGE_STYLE_DEFAULT,  0, 0, 100, 1);
	
	ido_scale_menu_item_set_style (IDO_SCALE_MENU_ITEM (priv->ido_volume_slider), IDO_SCALE_MENU_ITEM_STYLE_IMAGE);	
  g_object_set(priv->ido_volume_slider, "reverse-scroll-events", TRUE, NULL);

  g_signal_connect (volume_slider,
                    "notify::parent", G_CALLBACK (slider_parent_changed),
                    NULL);
	
  GtkWidget* volume_widget = ido_scale_menu_item_get_scale((IdoScaleMenuItem*)priv->ido_volume_slider);	
	
  // register slider changes listening on the range
	g_signal_connect(volume_widget, "change-value", G_CALLBACK(volume_widget_change_value_cb), self);	
	g_signal_connect(volume_widget, "value-changed", G_CALLBACK(volume_widget_value_changed_cb), self);	
  g_signal_connect(priv->ido_volume_slider, "slider-grabbed", G_CALLBACK(volume_widget_slider_grabbed), self);
  g_signal_connect(priv->ido_volume_slider, "slider-released", G_CALLBACK(volume_widget_slider_released), self);

	// Set images on the ido
  GtkWidget* primary_image = ido_scale_menu_item_get_primary_image((IdoScaleMenuItem*)priv->ido_volume_slider);
  GIcon * primary_gicon = g_themed_icon_new_with_default_fallbacks("audio-volume-low-zero-panel");
  gtk_image_set_from_gicon(GTK_IMAGE(primary_image), primary_gicon, GTK_ICON_SIZE_MENU);
  g_object_unref(primary_gicon);

  GtkWidget* secondary_image = ido_scale_menu_item_get_secondary_image((IdoScaleMenuItem*)priv->ido_volume_slider);
  GIcon * secondary_gicon = g_themed_icon_new_with_default_fallbacks("audio-volume-high-panel");
  gtk_image_set_from_gicon(GTK_IMAGE(secondary_image), secondary_gicon, GTK_ICON_SIZE_MENU);
  g_object_unref(secondary_gicon);

  GtkAdjustment *adj = gtk_range_get_adjustment (GTK_RANGE (volume_widget));
  gtk_adjustment_set_step_increment(adj, 3);

  gtk_widget_show_all(volume_widget);
}

static void
volume_widget_dispose (GObject *object)
{
	G_OBJECT_CLASS (volume_widget_parent_class)->dispose (object);
}

static void
volume_widget_finalize (GObject *object)
{
	G_OBJECT_CLASS (volume_widget_parent_class)->finalize (object);
}

static void 
volume_widget_property_update(DbusmenuMenuitem* item, gchar* property, 
                             GValue* value, gpointer userdata)
{
	g_debug("scrub-widget::property_update"); 
	
	g_return_if_fail (IS_VOLUME_WIDGET (userdata));	
	VolumeWidget* mitem = VOLUME_WIDGET(userdata);
	VolumeWidgetPrivate * priv = VOLUME_WIDGET_GET_PRIVATE(mitem);
	
}

static void
volume_widget_set_twin_item(VolumeWidget* self,
                           DbusmenuMenuitem* twin_item)
{
	VolumeWidgetPrivate * priv = VOLUME_WIDGET_GET_PRIVATE(self);
	priv->twin_item = twin_item;

	g_signal_connect(G_OBJECT(twin_item), "property-changed", 
	                 G_CALLBACK(volume_widget_property_update), self);

	/*volume_widget_set_ido_position(self, 
	                              dbusmenu_menuitem_property_get_int(priv->twin_item, DBUSMENU_SCRUB_MENUITEM_POSITION)/1000,
																dbusmenu_menuitem_property_get_int(priv->twin_item, DBUSMENU_SCRUB_MENUITEM_DURATION));	                                	                                
																*/
}

static gboolean
volume_widget_change_value_cb (GtkRange     *range,
                 							GtkScrollType scroll,
                 							gdouble       new_value,
                 							gpointer      user_data)
{
	g_return_val_if_fail (IS_VOLUME_WIDGET (user_data), FALSE);
	VolumeWidget* mitem = VOLUME_WIDGET(user_data);
	VolumeWidgetPrivate * priv = VOLUME_WIDGET_GET_PRIVATE(mitem);

  // Don't bother when the slider is grabbed  
  if(priv->grabbed == TRUE)
    return FALSE;

	GValue value = {0};
  g_value_init(&value, G_TYPE_DOUBLE);
	gdouble clamped = CLAMP(new_value, 0, 100);
  g_value_set_double(&value, clamped);
  dbusmenu_menuitem_handle_event (priv->twin_item, "grabbed", &value, 0);
	return TRUE;
}

static gboolean
volume_widget_value_changed(GtkRange *range, gpointer user_data)
{
	g_return_val_if_fail (IS_VOLUME_WIDGET (user_data), FALSE);
	VolumeWidget* mitem = VOLUME_WIDGET(user_data);
	VolumeWidgetPrivate * priv = VOLUME_WIDGET_GET_PRIVATE(mitem);
	
  gdouble current_value =  CLAMP(gtk_range_get_value(GTK_RANGE(priv->ido_volume_slider), 0, 100);

  // Replace with dbus properties inspection 
  /*if (current_value == exterior_vol_update) {
    g_debug("ignore the value changed event - its come from the outside");
    return FALSE;
  }*/
                                 
  GValue value = {0};
  g_value_init(&value, G_TYPE_DOUBLE);
  g_value_set_double(&value, current_value);
  g_debug("volume_widget_value changed callback - = %f", current_value);
  dbusmenu_menuitem_handle_event (priv->twin_item, "slider_change", &value, 0);
  // This is not ideal in that the icon ui will update on ui actions and not on actual service feedback.
  // but necessary for now as the server does not send volume update information if the source of change was this ui.
  determine_state_from_volume(current_value);
  return FALSE;
}


GtkWidget*
volume_widget_get_ido_bar(VolumeWidget* self)
{
	VolumeWidgetPrivate * priv = VOLUME_WIDGET_GET_PRIVATE(self);
	return priv->ido_volume_slider;	
}

static void
volume_widget_parent_changed (GtkWidget *widget,
                       				gpointer   user_data)
{
  gtk_widget_set_size_request (widget, 200, -1);
  g_debug("volume_widget_parent_changed");
}

 
static void
volume_widget_set_ido_position(VolumeWidget* self,
                              gint position,
                              gint duration)
{
	VolumeWidgetPrivate * priv = VOLUME_WIDGET_GET_PRIVATE(self);
	gdouble ido_position = position/(gdouble)duration	* 100.0;
	g_debug("volume_widget_set_ido_position - pos: %i, duration: %i, ido_pos: %f", position, duration, ido_position);
  GtkWidget *slider = ido_scale_menu_item_get_scale((IdoScaleMenuItem*)priv->ido_volume_slider);
  GtkRange *range = (GtkRange*)slider;
	if(duration == 0)
		ido_position = 0.0;
 	gtk_range_set_value(range, ido_position);
}

static void
volume_widget_slider_grabbed(GtkWidget *widget, gpointer user_data)
{
	VolumeWidget* mitem = VOLUME_WIDGET(user_data);
	VolumeWidgetPrivate * priv = VOLUME_WIDGET_GET_PRIVATE(mitem);
	priv->grabbed = TRUE;	
}

static void
volume_widget_slider_released(GtkWidget *widget, gpointer user_data)
{
	VolumeWidget* mitem = VOLUME_WIDGET(user_data);
	VolumeWidgetPrivate * priv = VOLUME_WIDGET_GET_PRIVATE(mitem);
	priv->grabbed = FALSE;
	GtkWidget *slider = ido_scale_menu_item_get_scale((IdoScaleMenuItem*)priv->ido_volume_slider);
  gdouble new_value = gtk_range_get_value(GTK_RANGE(slider));
	g_debug("okay set the scrub position with %f", new_value);	
	GValue value = {0};
  g_value_init(&value, G_TYPE_DOUBLE);
	gdouble clamped = CLAMP(new_value, 0, 100);
  g_value_set_double(&value, clamped);
  dbusmenu_menuitem_handle_event (priv->twin_item, "volume-change", &value, 0);
}


/**
 * volume_widget_new:
 * @returns: a new #VolumeWidget.
 **/
GtkWidget* 
volume_widget_new(DbusmenuMenuitem *item)
{
	GtkWidget* widget = g_object_new(VOLUME_WIDGET_TYPE, NULL);
	volume_widget_set_twin_item((VolumeWidget*)widget, item);
	return widget;
}


