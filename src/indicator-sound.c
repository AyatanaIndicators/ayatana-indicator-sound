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
/*#include <idoscalemenuitem.h>*/

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

#include <libindicator/indicator.h>
#include <libindicator/indicator-object.h>
#include <libindicator/indicator-service-manager.h>


#include "dbus-shared-names.h"
#include "sound-service-client.h"
#include "common-defs.h"
#include "sound-service-marshal.h"


#define INDICATOR_SOUND_TYPE            (indicator_sound_get_type ())
#define INDICATOR_SOUND(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), INDICATOR_SOUND_TYPE, IndicatorSound))
#define INDICATOR_SOUND_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), INDICATOR_SOUND_TYPE, IndicatorSoundClass))
#define IS_INDICATOR_SOUND(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), INDICATOR_SOUND_TYPE))
#define IS_INDICATOR_SOUND_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), INDICATOR_SOUND_TYPE))
#define INDICATOR_SOUND_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), INDICATOR_SOUND_TYPE, IndicatorSoundClass))

typedef struct _IndicatorSound      IndicatorSound;
typedef struct _IndicatorSoundClass IndicatorSoundClass;

struct _IndicatorSoundClass {
	IndicatorObjectClass parent_class;
};

struct _IndicatorSound {
	IndicatorObject parent;
	IndicatorServiceManager * service;
};

GType indicator_sound_get_type (void);


/* Indicator stuff */
INDICATOR_SET_VERSION
INDICATOR_SET_TYPE(INDICATOR_SOUND_TYPE)

/* Prototypes */
static GtkLabel * get_label (IndicatorObject * io);
static GtkImage * get_icon (IndicatorObject * io);
static GtkMenu * get_menu (IndicatorObject * io);
//static GtkWidget *volume_item;
static DBusGProxy * sound_dbus_proxy = NULL;


static void indicator_sound_class_init (IndicatorSoundClass *klass);
static void indicator_sound_init       (IndicatorSound *self);
static void indicator_sound_dispose    (GObject *object);
static void indicator_sound_finalize   (GObject *object);

G_DEFINE_TYPE (IndicatorSound, indicator_sound, INDICATOR_OBJECT_TYPE);

static void connection_changed (IndicatorServiceManager * sm, gboolean connected, gpointer userdata);
static void catch_signal(DBusGProxy * proxy, gint sink_index, gboolean value, gpointer userdata);

static void
indicator_sound_class_init (IndicatorSoundClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = indicator_sound_dispose;
	object_class->finalize = indicator_sound_finalize;

	IndicatorObjectClass * io_class = INDICATOR_OBJECT_CLASS(klass);
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
    g_debug("signal caught - i don't believe it !");
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
	GtkImage * status_image = GTK_IMAGE(gtk_image_new_from_icon_name("audio-volume-high", GTK_ICON_SIZE_MENU));
	gtk_widget_show(GTK_WIDGET(status_image));
	return status_image;
}

/* Indicator based function to get the menu for the whole
   applet.  This starts up asking for the parts of the menu
   from the various services. */
static GtkMenu *
get_menu (IndicatorObject * io)
{
    //volume_item = ido_scale_menu_item_new_with_range ("Volume", 0, 100, 1);
    //gtk_menu_shell_append (GTK_MENU_SHELL (menu), volume_item);
    return GTK_MENU(dbusmenu_gtkmenu_new(INDICATOR_SOUND_DBUS_NAME, INDICATOR_SOUND_DBUS_OBJECT));
}


