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
#include <libido/idoscalemenuitem.h>

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
//Slider related
static GtkWidget *volume_slider = NULL;
static gboolean new_slider_item (DbusmenuMenuitem * newitem, DbusmenuMenuitem * parent, DbusmenuClient * client);
static void slider_prop_change_cb (DbusmenuMenuitem * mi, gchar * prop, GValue * value, GtkWidget *widget);
static gboolean user_change_value_event_cb(GtkRange *range, GtkScrollType scroll_type, gdouble input_value, gpointer  user_data);
static gboolean value_changed_event_cb(GtkRange *range, gpointer user_data);

// DBUS communication
static DBusGProxy *sound_dbus_proxy = NULL;
static void connection_changed (IndicatorServiceManager * sm, gboolean connected, gpointer userdata);
static void catch_signal_sink_input_while_muted(DBusGProxy * proxy, gboolean value, gpointer userdata);
static void catch_signal_sink_volume_update(DBusGProxy * proxy, gdouble volume_percent, gpointer userdata); 
static void catch_signal_sink_mute_update(DBusGProxy *proxy, gboolean mute_value, gpointer userdata);
static void fetch_volume_percent_from_dbus();
static void fetch_mute_value_from_dbus();

/****Volume States 'members' ***/
static void prepare_state_machine();
static void determine_state_from_volume(gdouble volume_percent);
static void update_state(const gint state);
/*static void revert_state();*/
static const gint STATE_MUTED = 0;
static const gint STATE_ZERO = 1;
static const gint STATE_LOW = 2;
static const gint STATE_MEDIUM = 3;
static const gint STATE_HIGH = 4;
static const gint STATE_MUTED_WHILE_INPUT = 5;
static const gint STATE_SINKS_NONE = 5;
static GHashTable *volume_states = NULL;
static GtkImage *speaker_image = NULL;
static GtkWidget* primary_image = NULL;
static gint current_state = 0;
static gint previous_state = 0;
static gdouble initial_volume_percent = 0;
static gboolean initial_mute = FALSE;

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
    prepare_state_machine();
    return;
}


/*static void test_images_hash()*/
/*{*/
/*    g_debug("about to test the images hash");      */
/*    gchar* current_name = g_hash_table_lookup(volume_states, GINT_TO_POINTER(current_state));*/
/*    g_debug("start up current image name  = %s", current_name);       */
/*    gchar* previous_name = g_hash_table_lookup(volume_states, GINT_TO_POINTER(previous_state));*/
/*    g_debug("start up previous image name  = %s", previous_name);       */
/*}*/

/*
Prepare states Array.
*/
static void prepare_state_machine()
{
    // TODO we need three more images
    volume_states = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, g_free);
    g_hash_table_insert(volume_states, GINT_TO_POINTER(STATE_MUTED), g_strdup("audio-volume-muted"));
    g_hash_table_insert(volume_states, GINT_TO_POINTER(STATE_ZERO), g_strdup(/*"audio-volume-zero"*/"audio-volume-muted"));
    g_hash_table_insert(volume_states, GINT_TO_POINTER(STATE_LOW), g_strdup("audio-volume-low"));
    g_hash_table_insert(volume_states, GINT_TO_POINTER(STATE_MEDIUM), g_strdup("audio-volume-medium"));
    g_hash_table_insert(volume_states, GINT_TO_POINTER(STATE_HIGH), g_strdup("audio-volume-high"));
    g_hash_table_insert(volume_states, GINT_TO_POINTER(STATE_MUTED_WHILE_INPUT), g_strdup(/*"audio-volume-muted-blocking"*/"audio-volume-muted"));
    g_hash_table_insert(volume_states, GINT_TO_POINTER(STATE_SINKS_NONE), g_strdup(/*"audio-output-none"*/"audio-volume-muted"));
    //test_images_hash();
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
			dbus_g_proxy_add_signal(sound_dbus_proxy, SIGNAL_SINK_INPUT_WHILE_MUTED, G_TYPE_BOOLEAN, G_TYPE_INVALID);

			dbus_g_proxy_connect_signal(sound_dbus_proxy, SIGNAL_SINK_INPUT_WHILE_MUTED, G_CALLBACK(catch_signal_sink_input_while_muted), NULL, NULL);
			dbus_g_proxy_add_signal(sound_dbus_proxy, SIGNAL_SINK_VOLUME_UPDATE, G_TYPE_DOUBLE, G_TYPE_INVALID);
			dbus_g_proxy_connect_signal(sound_dbus_proxy, SIGNAL_SINK_VOLUME_UPDATE, G_CALLBACK(catch_signal_sink_volume_update), NULL, NULL);
			dbus_g_proxy_add_signal(sound_dbus_proxy, SIGNAL_SINK_MUTE_UPDATE, G_TYPE_BOOLEAN, G_TYPE_INVALID);
			dbus_g_proxy_connect_signal(sound_dbus_proxy, SIGNAL_SINK_MUTE_UPDATE, G_CALLBACK(catch_signal_sink_mute_update), NULL, NULL);

            // Ensure we are in a coherent state with the service at start up.
            // Preserve ordering!
            fetch_volume_percent_from_dbus();
            fetch_mute_value_from_dbus();
		}

	} else {
        //TODO : will need to handle this scenario
	}

	return;
}

static void fetch_volume_percent_from_dbus()
{
    GError * error = NULL;
    gdouble *volume_percent_input; 
    volume_percent_input = g_new0(gdouble, 1);
    org_ayatana_indicator_sound_get_sink_volume(sound_dbus_proxy, volume_percent_input, &error);
	if (error != NULL) {
		g_warning("Unable to fetch VOLUME at indicator start up: %s", error->message);
		g_error_free(error);
        g_free(volume_percent_input);
        return;
	}
    initial_volume_percent = *volume_percent_input;
    determine_state_from_volume(initial_volume_percent);
    g_free(volume_percent_input);
    g_debug("at the indicator start up and the volume percent returned from dbus method is %f", initial_volume_percent);
}

static void fetch_mute_value_from_dbus()
{
    GError * error = NULL;
    gboolean *mute_input; 
    mute_input = g_new0(gboolean, 1);
    org_ayatana_indicator_sound_get_sink_mute(sound_dbus_proxy, mute_input, &error);
    if (error != NULL) {
	    g_warning("Unable to fetch MUTE at indicator start up: %s", error->message);
	    g_error_free(error);
        g_free(mute_input);
        return;
    }
    initial_mute = *mute_input;
    if (initial_mute == TRUE)
        update_state(STATE_MUTED);
    g_free(mute_input);
    g_debug("at the indicator start up and the MUTE returned from dbus method is %i", initial_mute);
}

static void catch_signal_sink_input_while_muted(DBusGProxy * proxy, gboolean block_value, gpointer userdata)
{
    g_debug("signal caught - sink input while muted with value %i", block_value);
}

static void catch_signal_sink_volume_update(DBusGProxy *proxy, gdouble volume_percent, gpointer userdata)
{
    g_debug("signal caught - update sink volume with value : %f", volume_percent);
    GtkWidget *slider = ido_scale_menu_item_get_scale((IdoScaleMenuItem*)volume_slider);
    GtkRange *range = (GtkRange*)slider;   
    gtk_range_set_value(range, volume_percent); 
    determine_state_from_volume(volume_percent);
}

static void catch_signal_sink_mute_update(DBusGProxy *proxy, gboolean mute_value, gpointer userdata)
{
    //We can be sure the service won't send a mute signal unless it has changed !
    //UNMUTE's force a volume update therefore icon is updated appropriately => no need for unmute handling here.
    if(mute_value == TRUE)
    {
        g_debug("signal caught - sink mute update - MUTE");
        update_state(STATE_MUTED);
    }
}


static void
indicator_sound_dispose (GObject *object)
{
	IndicatorSound * self = INDICATOR_SOUND(object);

	if (self->service != NULL) {
		g_object_unref(G_OBJECT(self->service));
		self->service = NULL;
	}
    g_hash_table_destroy(volume_states);
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
    gchar* current_name = g_hash_table_lookup(volume_states, GINT_TO_POINTER(current_state));
    g_debug("At start-up attempting to set the image to %s", current_name);
	speaker_image = GTK_IMAGE(gtk_image_new_from_icon_name(current_name, GTK_ICON_SIZE_MENU));
	gtk_widget_show(GTK_WIDGET(speaker_image));
	return speaker_image;
}

static void update_state(const gint state)
{
    g_debug("update state beginning - previous_state = %i", previous_state);

    previous_state = current_state;

    g_debug("update state 3rd line - previous_state = %i", previous_state);

    current_state = state;
    gchar* image_name = g_hash_table_lookup(volume_states, GINT_TO_POINTER(current_state));
    gtk_image_set_from_icon_name(speaker_image, image_name, GTK_ICON_SIZE_MENU);
}

/*static void revert_state()*/
/*{*/

/*    g_debug("revert state beginning - previous_state = %i", previous_state);*/
/*    current_state = previous_state;*/
/*    gchar* image_name = g_hash_table_lookup(volume_states, GINT_TO_POINTER(current_state));*/
/*    gtk_image_set_from_icon_name(speaker_image, image_name, GTK_ICON_SIZE_MENU);*/
/*    g_debug("after reverting back to previous state of %i", current_state);*/
/*}*/

static void determine_state_from_volume(gdouble volume_percent)
{
    g_debug("determine_state_from_volume - previous_state = %i", previous_state);
    gint state = previous_state;
    if (volume_percent < 30.0 && volume_percent > 0){
        state = STATE_LOW;
    }
    else if(volume_percent < 70.0 && volume_percent > 30.0){
        state = STATE_MEDIUM;
    }
    else if(volume_percent > 70.0){
        state = STATE_HIGH;
    }
    else if(volume_percent == 0.0){
        state = STATE_ZERO;
    }    
    update_state(state);   
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
    
    volume_slider = ido_scale_menu_item_new_with_range ("Volume", initial_volume_percent, 0, 100, 0.5);

    GtkMenuItem *menu_volume_slider = GTK_MENU_ITEM(volume_slider);
	dbusmenu_gtkclient_newitem_base(DBUSMENU_GTKCLIENT(client), newitem, menu_volume_slider, parent);
	g_signal_connect(G_OBJECT(newitem), DBUSMENU_MENUITEM_SIGNAL_PROPERTY_CHANGED, G_CALLBACK(slider_prop_change_cb), volume_slider);
    
    GtkWidget* slider = ido_scale_menu_item_get_scale((IdoScaleMenuItem*)volume_slider);  
    g_signal_connect(slider, "change-value", G_CALLBACK(user_change_value_event_cb), newitem);     
    g_signal_connect(slider, "value-changed", G_CALLBACK(value_changed_event_cb), newitem);     
    
    primary_image = ido_scale_menu_item_get_primary_image((IdoScaleMenuItem*)volume_slider);    
    gtk_image_set_from_icon_name(GTK_IMAGE(primary_image), g_hash_table_lookup(volume_states, GINT_TO_POINTER(STATE_ZERO)), GTK_ICON_SIZE_MENU);
    GtkWidget* secondary_image = ido_scale_menu_item_get_secondary_image((IdoScaleMenuItem*)volume_slider);                 
    gtk_image_set_from_icon_name(GTK_IMAGE(secondary_image), g_hash_table_lookup(volume_states, GINT_TO_POINTER(STATE_HIGH)), GTK_ICON_SIZE_MENU);

/*    GtkRange* range = (GtkRange*)slider;   */
/*    gtk_range_set_value(range, initial_volume_percent);  */

    gtk_widget_show_all(volume_slider);
	return TRUE;
}

/* Whenever we have a property change on a DbusmenuMenuitem
   we need to be responsive to that. */
static void slider_prop_change_cb (DbusmenuMenuitem * mi, gchar * prop, GValue * value, GtkWidget *widget)
{
    g_debug("slider_prop_change_cb - dodgy updater ");
    g_debug("about to set the slider to %f", g_value_get_double(value));
    GtkWidget* slider = ido_scale_menu_item_get_scale((IdoScaleMenuItem*)volume_slider);
    GtkRange* range = (GtkRange*)slider;       
    gtk_range_set_value(range, g_value_get_double(value));  
	return;
}

/**
This callback will get triggered irregardless of whether its a user change or a programmatic change
Our usecase for this particular callback is only interested if the slider is changed by the user hitting either icon
which will result in a programmatic value change of 0 or 100 (work around).
**/
static gboolean value_changed_event_cb(GtkRange *range, gpointer user_data)
{
    gdouble current_value = gtk_range_get_value(range);        
    if(current_value == 0 || current_value == 100)
    {
        DbusmenuMenuitem *item = (DbusmenuMenuitem*)user_data;
        GValue value = {0};
        g_value_init(&value, G_TYPE_DOUBLE);
        g_value_set_double(&value, current_value);
        g_debug("Value changed listener - = %f", current_value);
        dbusmenu_menuitem_handle_event (item, "slider_change", &value, 0);        
    }
    return FALSE;
}

static gboolean user_change_value_event_cb(GtkRange *range, GtkScrollType scroll_type, gdouble input_value, gpointer  user_data)
{
    DbusmenuMenuitem *item = (DbusmenuMenuitem*)user_data;
    gdouble clamped_input = CLAMP(input_value, 0, 100);
    GValue value = {0};
    g_debug("User input on SLIDER - = %f", clamped_input);
    g_value_init(&value, G_TYPE_DOUBLE);
    g_value_set_double(&value, clamped_input);
    dbusmenu_menuitem_handle_event (item, "slider_change", &value, 0);
    return FALSE;  
} 


