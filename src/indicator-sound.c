/*
A small wrapper utility to load indicators and put them as menu items
into the gnome-panel using it's applet interface.

Copyright 2010 Canonical Ltd.

Authors:
    Conor Curran <conor.curra@canonical.com>
    Ted Gould <ted@canonical.com>

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

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <libdbusmenu-gtk/menu.h>
#include <idoscalemenuitem.h>

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

#include <libindicator/indicator.h>
#include <libindicator/indicator-object.h>
#include <libindicator/indicator-service-manager.h>


#include "dbus-shared-names.h"
#include "sound-service-client.h"
#include "common-defs.h"
#include "sound-service-marshal.h"

// GObject Boiler plate
#define INDICATOR_SOUND_TYPE            (indicator_sound_get_type ())
#define INDICATOR_SOUND(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), INDICATOR_SOUND_TYPE, IndicatorSound))
#define INDICATOR_SOUND_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), INDICATOR_SOUND_TYPE, IndicatorSoundClass))
#define IS_INDICATOR_SOUND(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), INDICATOR_SOUND_TYPE))
#define IS_INDICATOR_SOUND_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), INDICATOR_SOUND_TYPE))
#define INDICATOR_SOUND_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), INDICATOR_SOUND_TYPE, IndicatorSoundClass))

typedef struct _IndicatorSound      IndicatorSound;
typedef struct _IndicatorSoundClass IndicatorSoundClass;

//GObject class struct
struct _IndicatorSoundClass {
	IndicatorObjectClass parent_class;
};

//GObject instance struct
struct _IndicatorSound {
	IndicatorObject parent;
	IndicatorServiceManager *service;
};
// GObject Boiler plate
GType indicator_sound_get_type (void);
INDICATOR_SET_VERSION
INDICATOR_SET_TYPE(INDICATOR_SOUND_TYPE)

// GObject Boiler plate
static void indicator_sound_class_init (IndicatorSoundClass *klass);
static void indicator_sound_init       (IndicatorSound *self);
static void indicator_sound_dispose    (GObject *object);
static void indicator_sound_finalize   (GObject *object);
G_DEFINE_TYPE (IndicatorSound, indicator_sound, INDICATOR_OBJECT_TYPE);

//GTK+ items
static GtkLabel * get_label (IndicatorObject * io);
static GtkImage * get_icon (IndicatorObject * io);
static GtkMenu * get_menu (IndicatorObject * io);
static GtkWidget *volume_slider = NULL;
static gdouble input_value_from_across_the_dbus = 0.0;
static GtkImage *speaker_image = NULL;

static gboolean new_slider_item (DbusmenuMenuitem * newitem, DbusmenuMenuitem * parent, DbusmenuClient * client);
static void slider_prop_change_cb (DbusmenuMenuitem * mi, gchar * prop, GValue * value, GtkWidget *widget);
static gboolean slider_value_changed_event_cb(GtkRange *range, GtkScrollType scroll, double  value, gpointer  user_data);
static void change_speaker_image(gdouble volume_percent);

// DBUS communication
static DBusGProxy *sound_dbus_proxy = NULL;
static void connection_changed (IndicatorServiceManager * sm, gboolean connected, gpointer userdata);
static void catch_signal(DBusGProxy * proxy, gint sink_index, gboolean value, gpointer userdata);

static void
indicator_sound_class_init (IndicatorSoundClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = indicator_sound_dispose;
	object_class->finalize = indicator_sound_finalize;

	IndicatorObjectClass *io_class = INDICATOR_OBJECT_CLASS(klass);
	io_class->get_label = get_label;
	io_class->get_image = get_icon;
	io_class->get_menu = get_menu;

    dbus_g_object_register_marshaller (_sound_service_marshal_VOID__INT_BOOLEAN,
                                     G_TYPE_NONE,
                                     G_TYPE_INT,
                                     G_TYPE_BOOLEAN,
                                     G_TYPE_INVALID);
	return;
}

static void indicator_sound_init (IndicatorSound *self)
{
	/* Set good defaults */
	self->service = NULL;
	/* Now let's fire these guys up. */
	self->service = indicator_service_manager_new_version(INDICATOR_SOUND_DBUS_NAME, INDICATOR_SOUND_DBUS_VERSION);
	g_signal_connect(G_OBJECT(self->service), INDICATOR_SERVICE_MANAGER_SIGNAL_CONNECTION_CHANGE, G_CALLBACK(connection_changed), self);
    return;
}

static void
connection_changed (IndicatorServiceManager * sm, gboolean connected, gpointer userdata)
{
	if (connected) {
		if (sound_dbus_proxy == NULL) {
			GError * error = NULL;

			DBusGConnection * sbus = dbus_g_bus_get(DBUS_BUS_SESSION, NULL);

			sound_dbus_proxy = dbus_g_proxy_new_for_name_owner(sbus,
														   INDICATOR_SOUND_DBUS_NAME,
														   INDICATOR_SOUND_SERVICE_DBUS_OBJECT,
														   INDICATOR_SOUND_SERVICE_DBUS_INTERFACE,
														   &error);

			if (error != NULL) {
				g_warning("Unable to get status proxy: %s", error->message);
				g_error_free(error);
			}
            g_debug("about to connect to the signals");
			dbus_g_proxy_add_signal(sound_dbus_proxy, SIGNAL_SINK_INPUT_WHILE_MUTED, G_TYPE_INT, G_TYPE_BOOLEAN, G_TYPE_INVALID);
			dbus_g_proxy_connect_signal(sound_dbus_proxy, SIGNAL_SINK_INPUT_WHILE_MUTED, G_CALLBACK(catch_signal), NULL, NULL);
		}

	} else {
        //TODO : will need to handle this scenario
	}

	return;
}

static void catch_signal (DBusGProxy * proxy, gint sink_index, gboolean value, gpointer userdata)
{
    g_debug("signal caught - I don't believe it ! with index %i and value %i", sink_index, value);
}

static void
indicator_sound_dispose (GObject *object)
{
	IndicatorSound * self = INDICATOR_SOUND(object);

	if (self->service != NULL) {
		g_object_unref(G_OBJECT(self->service));
		self->service = NULL;
	}
	G_OBJECT_CLASS (indicator_sound_parent_class)->dispose (object);
	return;
}

static void
indicator_sound_finalize (GObject *object)
{
	G_OBJECT_CLASS (indicator_sound_parent_class)->finalize (object);
	return;
}

static GtkLabel *
get_label (IndicatorObject * io)
{
	return NULL;
}

static GtkImage *
get_icon (IndicatorObject * io)
{
	speaker_image = GTK_IMAGE(gtk_image_new_from_icon_name("audio-volume-high", GTK_ICON_SIZE_MENU));
	gtk_widget_show(GTK_WIDGET(speaker_image));
	return speaker_image;
}

static void change_speaker_image(gdouble volume_percent)
{    
    if (volume_percent < 30.0 && volume_percent > 0){
        gtk_image_set_from_icon_name(speaker_image, "audio-volume-low", GTK_ICON_SIZE_MENU);
    }
    else if(volume_percent < 70.0 && volume_percent > 30.0){
        gtk_image_set_from_icon_name(speaker_image, "audio-volume-medium", GTK_ICON_SIZE_MENU);
    }
    else if(volume_percent > 70.0){
        gtk_image_set_from_icon_name(speaker_image, "audio-volume-high", GTK_ICON_SIZE_MENU);
    }
    else if(volume_percent <= 0.0){
        gtk_image_set_from_icon_name(speaker_image, "audio-volume-muted", GTK_ICON_SIZE_MENU);
    }
}

/* Indicator based function to get the menu for the whole
   applet.  This starts up asking for the parts of the menu
   from the various services. */
static GtkMenu *
get_menu (IndicatorObject * io)
{
    DbusmenuGtkMenu *menu = dbusmenu_gtkmenu_new(INDICATOR_SOUND_DBUS_NAME, INDICATOR_SOUND_DBUS_OBJECT);    
	DbusmenuGtkClient *client = dbusmenu_gtkmenu_get_client(menu);	
    dbusmenu_client_add_type_handler(DBUSMENU_CLIENT(client), DBUSMENU_SLIDER_MENUITEM_TYPE, new_slider_item);

    return GTK_MENU(menu);
}

static gboolean new_slider_item(DbusmenuMenuitem * newitem, DbusmenuMenuitem * parent, DbusmenuClient * client)
{
	g_return_val_if_fail(DBUSMENU_IS_MENUITEM(newitem), FALSE);
	g_return_val_if_fail(DBUSMENU_IS_GTKCLIENT(client), FALSE);
    
    volume_slider = ido_scale_menu_item_new_with_range ("Volume", 0, 100, 0.5);

    GtkMenuItem *menu_volume_slider = GTK_MENU_ITEM(volume_slider);
	dbusmenu_gtkclient_newitem_base(DBUSMENU_GTKCLIENT(client), newitem, menu_volume_slider, parent);
	g_signal_connect(G_OBJECT(newitem), DBUSMENU_MENUITEM_SIGNAL_PROPERTY_CHANGED, G_CALLBACK(slider_prop_change_cb), volume_slider);

    GtkWidget* slider = ido_scale_menu_item_get_scale((IdoScaleMenuItem*)volume_slider);
    GtkRange* range = (GtkRange*)slider;   
    g_signal_connect(G_OBJECT(range), "change-value", G_CALLBACK(slider_value_changed_event_cb), newitem); 
	return TRUE;
}

/* Whenever we have a property change on a DbusmenuMenuitem
   we need to be responsive to that. */
static void slider_prop_change_cb (DbusmenuMenuitem * mi, gchar * prop, GValue * value, GtkWidget *widget)
{
    g_debug("slider_prop_change_cb ");
    g_debug("about to set the slider to %f", g_value_get_double(value));
    GtkWidget* slider = ido_scale_menu_item_get_scale((IdoScaleMenuItem*)volume_slider);
    GtkRange* range = (GtkRange*)slider;   
    gdouble level;
    level = gtk_range_get_fill_level(range);
    input_value_from_across_the_dbus = level;
    g_debug("the current level is %f", level);
    
    gtk_range_set_value(range, g_value_get_double(value));  
	return;
}

static gboolean slider_value_changed_event_cb(GtkRange *range, GtkScrollType scroll, double  slider_value, gpointer  user_data)
{
    if(slider_value != input_value_from_across_the_dbus)
    {    
        DbusmenuMenuitem *item = (DbusmenuMenuitem*)user_data;
        GValue value = {0};
        g_value_init(&value, G_TYPE_DOUBLE);
        g_value_set_double(&value, slider_value);
        dbusmenu_menuitem_handle_event (item, "slider_change", &value, 0);
        change_speaker_image(slider_value);
    }
    return FALSE;  
} 


