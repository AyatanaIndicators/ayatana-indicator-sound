/*
A small wrapper utility to load indicators and put them as menu items
into the gnome-panel using it's applet interface.

Copyright 2010 Canonical Ltd.

Authors:
    Conor Curran <conor.curran@canonical.com>
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
#include <math.h>
#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <libdbusmenu-gtk/menu.h>
#include <libido/idoscalemenuitem.h>

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

#include <libindicator/indicator.h>
#include <libindicator/indicator-object.h>
#include <libindicator/indicator-service-manager.h>
#include <libindicator/indicator-image-helper.h>

#include "indicator-sound.h"
#include "dbus-shared-names.h"
#include "sound-service-client.h"
#include "common-defs.h"

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
/*static void slider_prop_change_cb (DbusmenuMenuitem * mi, gchar * prop, GValue * value, GtkWidget *widget);*/
static gboolean value_changed_event_cb(GtkRange *range, gpointer user_data);
static gboolean key_press_cb(GtkWidget* widget, GdkEventKey* event, gpointer data);
/*static void slider_size_allocate(GtkWidget  *widget, GtkAllocation *allocation, gpointer user_data);*/
static void slider_grabbed(GtkWidget *widget, gpointer user_data);
static void slider_released(GtkWidget *widget, gpointer user_data);

// DBUS communication
static DBusGProxy *sound_dbus_proxy = NULL;
static void connection_changed (IndicatorServiceManager * sm, gboolean connected, gpointer userdata);
static void catch_signal_sink_input_while_muted(DBusGProxy * proxy, gboolean value, gpointer userdata);
static void catch_signal_sink_volume_update(DBusGProxy * proxy, gdouble volume_percent, gpointer userdata);
static void catch_signal_sink_mute_update(DBusGProxy *proxy, gboolean mute_value, gpointer userdata);
static void catch_signal_sink_availability_update(DBusGProxy *proxy, gboolean available_value, gpointer userdata);
static void fetch_volume_percent_from_dbus();
static void fetch_mute_value_from_dbus();
static void fetch_sink_availability_from_dbus();

/****Volume States 'members' ***/
static void update_state(const gint state);

static const gint STATE_MUTED = 0;
static const gint STATE_ZERO = 1;
static const gint STATE_LOW = 2;
static const gint STATE_MEDIUM = 3;
static const gint STATE_HIGH = 4;
static const gint STATE_MUTED_WHILE_INPUT = 5;
static const gint STATE_SINKS_NONE = 6;

static GHashTable *volume_states = NULL;
static GtkImage *speaker_image = NULL;
static gint current_state = 0;
static gint previous_state = 0;

static gdouble initial_volume_percent = 0;
static gboolean initial_mute = FALSE;
static gboolean device_available = TRUE;
static gboolean slider_in_direct_use = FALSE;

static GtkIconSize design_team_size;
static gint animation_id;
static GList * blocked_animation_list = NULL;
static GList * blocked_iter = NULL;
static void prepare_blocked_animation();
static gboolean fade_back_to_mute_image();

// Construction
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

    design_team_size = gtk_icon_size_register("design-team-size", 22, 22);

	return;
}

static void indicator_sound_init (IndicatorSound *self)
{
	self->service = NULL;
	self->service = indicator_service_manager_new_version(INDICATOR_SOUND_DBUS_NAME, INDICATOR_SOUND_DBUS_VERSION);
	g_signal_connect(G_OBJECT(self->service), INDICATOR_SERVICE_MANAGER_SIGNAL_CONNECTION_CHANGE, G_CALLBACK(connection_changed), self);
    prepare_state_machine();
    prepare_blocked_animation();
    animation_id = 0;
    return;
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
    
    if(blocked_animation_list != NULL){
        g_list_foreach (blocked_animation_list, (GFunc)g_object_unref, NULL);
        g_list_free(blocked_animation_list);
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
    gchar* current_name = g_hash_table_lookup(volume_states, GINT_TO_POINTER(current_state));
    //g_debug("At start-up attempting to set the image to %s", current_name);
	speaker_image = indicator_image_helper(current_name);
	gtk_widget_show(GTK_WIDGET(speaker_image));
	return speaker_image;
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

    // register Key-press listening on the menu widget as the slider does not allow this.
    g_signal_connect(menu, "key-press-event", G_CALLBACK(key_press_cb), NULL);
    return GTK_MENU(menu);
}

static void
slider_parent_changed (GtkWidget *widget,
                       gpointer   user_data)
{
    gtk_widget_set_size_request (widget, 200, -1);
}

/**
new_slider_item:
Create a new dBusMenu Slider item.
**/
static gboolean new_slider_item(DbusmenuMenuitem * newitem, DbusmenuMenuitem * parent, DbusmenuClient * client)
{
    g_return_val_if_fail(DBUSMENU_IS_MENUITEM(newitem), FALSE);
    g_return_val_if_fail(DBUSMENU_IS_GTKCLIENT(client), FALSE);

    volume_slider = ido_scale_menu_item_new_with_range ("Volume", initial_volume_percent, 0, 100, 0.5);
    g_object_set(volume_slider, "reverse-scroll-events", TRUE, NULL);

    g_signal_connect (volume_slider,
                      "notify::parent", G_CALLBACK (slider_parent_changed),
                      NULL);

    GtkMenuItem *menu_volume_slider = GTK_MENU_ITEM(volume_slider);

    dbusmenu_gtkclient_newitem_base(DBUSMENU_GTKCLIENT(client), newitem, menu_volume_slider, parent);

    // register slider changes listening on the range
    GtkWidget* slider = ido_scale_menu_item_get_scale((IdoScaleMenuItem*)volume_slider);

    g_signal_connect(slider, "value-changed", G_CALLBACK(value_changed_event_cb), newitem);
    g_signal_connect(volume_slider, "slider-grabbed", G_CALLBACK(slider_grabbed), NULL);
    g_signal_connect(volume_slider, "slider-released", G_CALLBACK(slider_released), NULL);
/*    g_signal_connect(slider, "size-allocate", G_CALLBACK(slider_size_allocate), NULL);*/

    // Set images on the ido
    GtkWidget* primary_image = ido_scale_menu_item_get_primary_image((IdoScaleMenuItem*)volume_slider);
    GIcon * primary_gicon = g_themed_icon_new_with_default_fallbacks(g_hash_table_lookup(volume_states, GINT_TO_POINTER(STATE_ZERO)));
    gtk_image_set_from_gicon(GTK_IMAGE(primary_image), primary_gicon, GTK_ICON_SIZE_MENU);
    g_object_unref(primary_gicon);

    GtkWidget* secondary_image = ido_scale_menu_item_get_secondary_image((IdoScaleMenuItem*)volume_slider);
    GIcon * secondary_gicon = g_themed_icon_new_with_default_fallbacks(g_hash_table_lookup(volume_states, GINT_TO_POINTER(STATE_HIGH)));
    gtk_image_set_from_gicon(GTK_IMAGE(secondary_image), secondary_gicon, GTK_ICON_SIZE_MENU);
    g_object_unref(secondary_gicon);

    gtk_widget_set_sensitive(volume_slider, !initial_mute);
    gtk_widget_show_all(volume_slider);

    return TRUE;
}

static void
connection_changed (IndicatorServiceManager * sm, gboolean connected, gpointer userdata)
{
    // TODO: This could be safer.
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
			dbus_g_proxy_add_signal(sound_dbus_proxy, SIGNAL_SINK_AVAILABLE_UPDATE, G_TYPE_BOOLEAN, G_TYPE_INVALID);
			dbus_g_proxy_connect_signal(sound_dbus_proxy, SIGNAL_SINK_AVAILABLE_UPDATE, G_CALLBACK(catch_signal_sink_availability_update), NULL, NULL);

            // Ensure we are in a coherent state with the service at start up.
            // Preserve ordering!
            fetch_volume_percent_from_dbus();
            fetch_mute_value_from_dbus();
            fetch_sink_availability_from_dbus();
		}

	} else {
        //TODO : will need to handle this scenario
	}

	return;
}

/*
Prepare states Array.
*/
void prepare_state_machine()
{
    volume_states = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, g_free);
    g_hash_table_insert(volume_states, GINT_TO_POINTER(STATE_MUTED), g_strdup("audio-volume-muted-panel"));
    g_hash_table_insert(volume_states, GINT_TO_POINTER(STATE_ZERO), g_strdup("audio-volume-low-zero-panel"));
    g_hash_table_insert(volume_states, GINT_TO_POINTER(STATE_LOW), g_strdup("audio-volume-low-panel"));
    g_hash_table_insert(volume_states, GINT_TO_POINTER(STATE_MEDIUM), g_strdup("audio-volume-medium-panel"));
    g_hash_table_insert(volume_states, GINT_TO_POINTER(STATE_HIGH), g_strdup("audio-volume-high-panel"));
    g_hash_table_insert(volume_states, GINT_TO_POINTER(STATE_MUTED_WHILE_INPUT), g_strdup("audio-volume-muted-blocking-panel"));
    g_hash_table_insert(volume_states, GINT_TO_POINTER(STATE_SINKS_NONE), g_strdup("audio-output-none-panel"));
}

/*
prepare_blocked_animation:
Prepares the array of images to be used in the blocked animation.
Only called at startup.
*/
static void prepare_blocked_animation()
{
    gchar* blocked_name = g_hash_table_lookup(volume_states, GINT_TO_POINTER(STATE_MUTED_WHILE_INPUT));
    gchar* muted_name = g_hash_table_lookup(volume_states, GINT_TO_POINTER(STATE_MUTED));
    
    GtkImage* temp_image = indicator_image_helper(muted_name);       
    GdkPixbuf* mute_buf = gtk_image_get_pixbuf(temp_image); 

    temp_image = indicator_image_helper(blocked_name);       
    GdkPixbuf* blocked_buf = gtk_image_get_pixbuf(temp_image);

    int i;

    if(mute_buf == NULL || blocked_buf == NULL){
        g_debug("Don bother with the animation, the theme aint got the goods");
        return;        
    }

    // sample 22 snapshots - range : 0-256
    for(i = 0; i < 23; i++)
    {
        gdk_pixbuf_composite(mute_buf, blocked_buf, 0, 0,
                             gdk_pixbuf_get_width(mute_buf),
                             gdk_pixbuf_get_height(mute_buf),
                             0, 0, 1, 1, GDK_INTERP_BILINEAR, MIN(255, i * 11));
        blocked_animation_list = g_list_append(blocked_animation_list, gdk_pixbuf_copy(blocked_buf));
    }
}


gint get_state()
{
    return current_state;
}

gchar* get_state_image_name(gint state)
{
    return g_hash_table_lookup(volume_states, GINT_TO_POINTER(state));
}

void prepare_for_tests(IndicatorObject *io)
{
    prepare_state_machine();
    get_icon(io);
}

void tidy_up_hash()
{
    g_hash_table_destroy(volume_states);
}

static void update_state(const gint state)
{
/*    g_debug("update state beginning - previous_state = %i", previous_state);*/

    previous_state = current_state;

/*    g_debug("update state 3rd line - previous_state = %i", previous_state);*/

    current_state = state;
    gchar* image_name = g_hash_table_lookup(volume_states, GINT_TO_POINTER(current_state));
	GtkImage * tempimage = indicator_image_helper(image_name);
    gtk_image_set_from_pixbuf(speaker_image, gtk_image_get_pixbuf(tempimage));
	g_object_ref_sink(tempimage);
}


void determine_state_from_volume(gdouble volume_percent)
{
/*    g_debug("determine_state_from_volume - previous_state = %i", previous_state);*/
    if (device_available == FALSE)
        return;
    gint state = previous_state;
    if (volume_percent < 30.0 && volume_percent > 0){
        state = STATE_LOW;
    }
    else if(volume_percent < 70.0 && volume_percent >= 30.0){
        state = STATE_MEDIUM;
    }
    else if(volume_percent >= 70.0){
        state = STATE_HIGH;
    }
    else if(volume_percent == 0.0){
        state = STATE_ZERO;
    }
    update_state(state);
}


static void fetch_sink_availability_from_dbus()
{
    GError * error = NULL;
    gboolean * available_input;
    available_input = g_new0(gboolean, 1);
    org_ayatana_indicator_sound_get_sink_availability(sound_dbus_proxy, available_input, &error);
    if (error != NULL) {
	    g_warning("Unable to fetch AVAILABILITY at indicator start up: %s", error->message);
	    g_error_free(error);
        g_free(available_input);
        return;
    }
    device_available = *available_input;
    if (device_available == FALSE)
        update_state(STATE_SINKS_NONE);
    g_free(available_input);
    g_debug("IndicatorSound::fetch_sink_availability_from_dbus -> AVAILABILTY returned from dbus method is %i", device_available);

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
    if (block_value == 1 && animation_id == 0 && blocked_animation_list != NULL) {
        gchar* image_name = g_hash_table_lookup(volume_states, GINT_TO_POINTER(STATE_MUTED_WHILE_INPUT));
    	GtkImage * tempimage = indicator_image_helper(image_name);
        gtk_image_set_from_pixbuf(speaker_image, gtk_image_get_pixbuf(tempimage));
        g_object_ref_sink(tempimage);

        blocked_iter = blocked_animation_list;
        animation_id = g_timeout_add_seconds(1, fade_back_to_mute_image, NULL);
    }  
}

static gboolean fade_back_to_mute_image()
{
    if(blocked_iter != NULL)
    {
        g_debug("in animation 'loop'\n");
        gtk_image_set_from_pixbuf(speaker_image, blocked_iter->data);
        blocked_iter = blocked_iter->next;
        return TRUE;
    }
    else{
        animation_id = 0;
        g_debug("exit from animation\n");
        return FALSE;
    }
}

static void catch_signal_sink_volume_update(DBusGProxy *proxy, gdouble volume_percent, gpointer userdata)
{
    if (slider_in_direct_use != TRUE){
        GtkWidget *slider = ido_scale_menu_item_get_scale((IdoScaleMenuItem*)volume_slider);
        GtkRange *range = (GtkRange*)slider;

        // DEBUG
        gdouble current_value = gtk_range_get_value(range);
        g_debug("SIGNAL- update sink volume - current_value : %f and new value : %f", current_value, volume_percent);
        gtk_range_set_value(range, volume_percent);
        determine_state_from_volume(volume_percent);
    }
}

static void catch_signal_sink_mute_update(DBusGProxy *proxy, gboolean mute_value, gpointer userdata)
{
    //We can be sure the service won't send a mute signal unless it has changed !
    //UNMUTE's force a volume update therefore icon is updated appropriately => no need for unmute handling here.
    if(mute_value == TRUE && device_available != FALSE)
    {
        update_state(STATE_MUTED);
    }
    else{
        if(animation_id != 0){
            g_debug("about to remove the animation_id callback from the mainloop!!**");
            g_source_remove(animation_id);
            animation_id = 0;
        }
    }
    g_debug("signal caught - sink mute update with mute value: %i", mute_value);
    gtk_widget_set_sensitive(volume_slider, !mute_value);
}

static void catch_signal_sink_availability_update(DBusGProxy *proxy, gboolean available_value, gpointer userdata)
{
    device_available  = available_value;
    if (device_available == FALSE){
        update_state(STATE_SINKS_NONE);
    }
    g_debug("signal caught - sink availability update with  value: %i", available_value);
}


/**
value_changed_event_cb:
This callback will get triggered irregardless of whether its a user change or a programmatic change.
**/
static gboolean value_changed_event_cb(GtkRange *range, gpointer user_data)
{
    gdouble current_value =  CLAMP(gtk_range_get_value(range), 0, 100);
    DbusmenuMenuitem *item = (DbusmenuMenuitem*)user_data;
    GValue value = {0};
    g_value_init(&value, G_TYPE_DOUBLE);
    g_value_set_double(&value, current_value);
    g_debug("Value changed callback - = %f", current_value);
    dbusmenu_menuitem_handle_event (item, "slider_change", &value, 0);
    // This is not ideal in that the icon ui will update on ui actions and not on actual service feedback.
    // but necessary for now as the server does not send volume update information if the source of change was this ui.
    determine_state_from_volume(current_value);
    return FALSE;
}


static void slider_grabbed (GtkWidget *widget, gpointer user_data)
{
    slider_in_direct_use = TRUE;
    g_debug ("!!!!!!  grabbed\n");
}

static void slider_released (GtkWidget *widget, gpointer user_data)
{
    slider_in_direct_use = FALSE;
    g_debug ("!!!!!! released\n");
}


/**
slider_size_allocate:
Callback on the size-allocate event on the slider item.
**/
/*static void slider_size_allocate(GtkWidget  *widget,*/
/*                                 GtkAllocation *allocation, */
/*                                 gpointer user_data)*/
/*{*/
/*    g_print("Size allocate on slider (%dx%d)\n", allocation->width, allocation->height);*/
/*    if(allocation->width < 200){*/
/*        g_print("Attempting to resize the slider");*/
/*        gtk_widget_set_size_request(widget, 200, -1);    */
/*    }*/
/*}*/

/**
key_press_cb:
**/
static gboolean key_press_cb(GtkWidget* widget, GdkEventKey* event, gpointer data)
{
    gboolean digested = FALSE;

    GtkWidget* slider = ido_scale_menu_item_get_scale((IdoScaleMenuItem*)volume_slider);
    GtkRange* range = (GtkRange*)slider;
    gdouble current_value = gtk_range_get_value(range);
    gdouble new_value = current_value;
    const gdouble five_percent = 5;
    GtkWidget *menuitem;

    menuitem = GTK_MENU_SHELL (widget)->active_menu_item;
    if(IDO_IS_SCALE_MENU_ITEM(menuitem) == TRUE)
    {
        switch(event->keyval)
            {
            case GDK_Right:
                digested = TRUE;
                if(event->state & GDK_CONTROL_MASK)
                {
                    new_value = 100;
                }
                else
                {
                    new_value = current_value + five_percent;
                }
                break;
            case GDK_Left:
                digested = TRUE;
                if(event->state & GDK_CONTROL_MASK)
                {
                    new_value = 0;
                }
                else
                {
                    new_value = current_value - five_percent;
                }
                break;
            case GDK_plus:
                digested = TRUE;
                new_value = current_value + five_percent;
                break;
            case GDK_minus:
                digested = TRUE;
                new_value = current_value - five_percent;
                break;
            default:
                break;
            }

            new_value = CLAMP(new_value, 0, 100);
            if(new_value != current_value && current_state != STATE_MUTED)
            {
                g_debug("Attempting to set the range from the key listener to %f", new_value);
                gtk_range_set_value(range, new_value);
            }
    }
    return digested;
}


