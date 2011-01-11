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

#include <gio/gio.h>

#include "indicator-sound.h"
#include "transport-widget.h"
#include "metadata-widget.h"
#include "title-widget.h"
#include "volume-widget.h"

#include "dbus-shared-names.h"

#include "gen-sound-service.xml.h"
#include "common-defs.h"

typedef struct _IndicatorSoundPrivate IndicatorSoundPrivate;

struct _IndicatorSoundPrivate
{
  GtkWidget* volume_widget;
  GList* transport_widgets_list;
  GDBusProxy *dbus_proxy;  
};

#define INDICATOR_SOUND_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), INDICATOR_SOUND_TYPE, IndicatorSoundPrivate))

// GObject Boiler plate
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
static GtkMenu *  get_menu (IndicatorObject * io);
static void       indicator_sound_scroll (IndicatorObject* io, gint delta, IndicatorScrollDirection direction);

//Slider related
static gboolean new_volume_slider_widget(DbusmenuMenuitem * newitem, DbusmenuMenuitem * parent, DbusmenuClient * client);
static gboolean key_press_cb(GtkWidget* widget, GdkEventKey* event, gpointer data);
static gboolean key_release_cb(GtkWidget* widget, GdkEventKey* event, gpointer data);
static void style_changed_cb(GtkWidget *widget, gpointer user_data);

//player widget realisation methods
static gboolean new_transport_widget(DbusmenuMenuitem * newitem, DbusmenuMenuitem * parent, DbusmenuClient * client);
static gboolean new_metadata_widget(DbusmenuMenuitem * newitem, DbusmenuMenuitem * parent, DbusmenuClient * client);
static gboolean new_title_widget(DbusmenuMenuitem * newitem, DbusmenuMenuitem * parent, DbusmenuClient * client);

// DBUS communication

static GDBusNodeInfo *node_info = NULL;
static GDBusInterfaceInfo *interface_info = NULL;
static void create_connection_to_service (GObject *source_object,
                                          GAsyncResult *res,
                                          gpointer user_data);
static void connection_changed (IndicatorServiceManager * sm,
                                gboolean connected,
                                gpointer userdata);
static void g_signal_cb ( GDBusProxy* proxy,
                          gchar* sender_name,
                          gchar* signal_name,
                          GVariant* parameters,
                          gpointer user_data);

static void react_to_signal_sink_input_while_muted (gboolean value,
                                                    IndicatorSound* self);
static void react_to_signal_sink_mute_update (gboolean value,
                                              IndicatorSound* self);
static void react_to_signal_sink_availability_update (gboolean value,
                                                      IndicatorSound* self);
static void fetch_state ( IndicatorSound* self );

static void get_sink_mute_cb ( GObject *object,
                               GAsyncResult *res,
                               gpointer user_data );

static void get_sink_availability_cb ( GObject *object,
                                       GAsyncResult *res,
                                       gpointer user_data );

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

static gboolean initial_mute ;
static gboolean device_available;

static GtkIconSize design_team_size;
static gint blocked_id;
static gint animation_id;

static GList * blocked_animation_list = NULL;
static GList * blocked_iter = NULL;
static void prepare_blocked_animation();
static gboolean fade_back_to_mute_image();
static gboolean start_animation();
static void reset_mute_blocking_animation();
static void free_the_animation_list();


static void
indicator_sound_class_init (IndicatorSoundClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = indicator_sound_dispose;
  object_class->finalize = indicator_sound_finalize;

  IndicatorObjectClass *io_class = INDICATOR_OBJECT_CLASS(klass);

  g_type_class_add_private (klass, sizeof (IndicatorSoundPrivate));
  
  io_class->get_label = get_label;
  io_class->get_image = get_icon;
  io_class->get_menu  = get_menu;
  io_class->scroll    = indicator_sound_scroll;
  design_team_size = gtk_icon_size_register("design-team-size", 22, 22);
}

static void
indicator_sound_init (IndicatorSound *self)
{
  self->service = NULL;
  self->service = indicator_service_manager_new_version(INDICATOR_SOUND_DBUS_NAME,
                                                        INDICATOR_SOUND_DBUS_VERSION);

  prepare_state_machine();
  prepare_blocked_animation();
  animation_id = 0;
  blocked_id = 0;
  initial_mute = FALSE;
  device_available = TRUE;
  
  IndicatorSoundPrivate* priv = INDICATOR_SOUND_GET_PRIVATE(self);
  priv->volume_widget = NULL;
  priv->dbus_proxy = NULL;
  GList* t_list = NULL;
  priv->transport_widgets_list = t_list;
  
  g_signal_connect(G_OBJECT(self->service),
                   INDICATOR_SERVICE_MANAGER_SIGNAL_CONNECTION_CHANGE,
                   G_CALLBACK(connection_changed), self);
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

  free_the_animation_list();

  IndicatorSoundPrivate* priv = INDICATOR_SOUND_GET_PRIVATE(INDICATOR_SOUND (self));

  g_list_free ( priv->transport_widgets_list );

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
  gchar* current_name = g_hash_table_lookup(volume_states,
                                            GINT_TO_POINTER(current_state));
  //g_debug("At start-up attempting to set the image to %s",
  //        current_name);
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
  DbusmenuGtkMenu* menu = dbusmenu_gtkmenu_new(INDICATOR_SOUND_DBUS_NAME,
                                               INDICATOR_SOUND_MENU_DBUS_OBJECT_PATH);
        
  DbusmenuGtkClient *client = dbusmenu_gtkmenu_get_client(menu);
  g_object_set_data (G_OBJECT (client), "indicator", io);
  dbusmenu_client_add_type_handler(DBUSMENU_CLIENT(client), DBUSMENU_VOLUME_MENUITEM_TYPE, new_volume_slider_widget);
  dbusmenu_client_add_type_handler(DBUSMENU_CLIENT(client), DBUSMENU_TRANSPORT_MENUITEM_TYPE, new_transport_widget);
  dbusmenu_client_add_type_handler(DBUSMENU_CLIENT(client), DBUSMENU_METADATA_MENUITEM_TYPE, new_metadata_widget);
  dbusmenu_client_add_type_handler(DBUSMENU_CLIENT(client), DBUSMENU_TITLE_MENUITEM_TYPE, new_title_widget);
  // register Key-press listening on the menu widget as the slider does not allow this.
  g_signal_connect(menu, "key-press-event", G_CALLBACK(key_press_cb), io);
  g_signal_connect(menu, "key-release-event", G_CALLBACK(key_release_cb), io);
  
  return GTK_MENU(menu);
}

static void
free_the_animation_list()
{
  if (blocked_animation_list != NULL) {
    g_list_foreach (blocked_animation_list, (GFunc)g_object_unref, NULL);
    g_list_free(blocked_animation_list);
    blocked_animation_list = NULL;
  }
}

static gboolean
new_transport_widget(DbusmenuMenuitem * newitem, DbusmenuMenuitem * parent, DbusmenuClient * client)
{
  g_debug("indicator-sound: new_transport_bar() called ");

  GtkWidget* bar = NULL;
  IndicatorObject *io = NULL;

  g_return_val_if_fail(DBUSMENU_IS_MENUITEM(newitem), FALSE);
  g_return_val_if_fail(DBUSMENU_IS_GTKCLIENT(client), FALSE);

  bar = transport_widget_new(newitem);
  io = g_object_get_data (G_OBJECT (client), "indicator");
  IndicatorSoundPrivate* priv = INDICATOR_SOUND_GET_PRIVATE(INDICATOR_SOUND (io));
  priv->transport_widgets_list = g_list_append ( priv->transport_widgets_list, bar );

  GtkMenuItem *menu_transport_bar = GTK_MENU_ITEM(bar);

  gtk_widget_show_all(bar);
  dbusmenu_gtkclient_newitem_base(DBUSMENU_GTKCLIENT(client), newitem, menu_transport_bar, parent);

  return TRUE;
}

static gboolean
new_metadata_widget(DbusmenuMenuitem * newitem, DbusmenuMenuitem * parent, DbusmenuClient * client)
{
  g_debug("indicator-sound: new_metadata_widget");

  GtkWidget* metadata = NULL;

  g_return_val_if_fail(DBUSMENU_IS_MENUITEM(newitem), FALSE);
  g_return_val_if_fail(DBUSMENU_IS_GTKCLIENT(client), FALSE);

  metadata = metadata_widget_new (newitem);
  GtkMenuItem *menu_metadata_widget = GTK_MENU_ITEM(metadata);

  gtk_widget_show_all(metadata);
  dbusmenu_gtkclient_newitem_base(DBUSMENU_GTKCLIENT(client), newitem, menu_metadata_widget, parent);

  return TRUE;
}

static gboolean
new_title_widget(DbusmenuMenuitem * newitem, DbusmenuMenuitem * parent, DbusmenuClient * client)
{
  g_return_val_if_fail(DBUSMENU_IS_MENUITEM(newitem), FALSE);
  g_return_val_if_fail(DBUSMENU_IS_GTKCLIENT(client), FALSE);

  g_debug ("%s (\"%s\")", __func__, dbusmenu_menuitem_property_get(newitem, DBUSMENU_TITLE_MENUITEM_NAME));

  GtkWidget* title = NULL;

  title = title_widget_new (newitem);
  GtkMenuItem *menu_title_widget = GTK_MENU_ITEM(title);
  
  gtk_widget_show_all(title);

  dbusmenu_gtkclient_newitem_base(DBUSMENU_GTKCLIENT(client),
                                  newitem,
                                  menu_title_widget, parent); 
  return TRUE;
}

static gboolean
new_volume_slider_widget(DbusmenuMenuitem * newitem, DbusmenuMenuitem * parent, DbusmenuClient * client)
{
  g_debug("indicator-sound: new_volume_slider_widget");

  GtkWidget* volume_widget = NULL;
  IndicatorObject *io = NULL;

  g_return_val_if_fail(DBUSMENU_IS_MENUITEM(newitem), FALSE);
  g_return_val_if_fail(DBUSMENU_IS_GTKCLIENT(client), FALSE);

  volume_widget = volume_widget_new (newitem);  
  io = g_object_get_data (G_OBJECT (client), "indicator");
  IndicatorSoundPrivate* priv = INDICATOR_SOUND_GET_PRIVATE(INDICATOR_SOUND (io));
  priv->volume_widget = volume_widget;

  GtkWidget* ido_slider_widget = volume_widget_get_ido_slider(VOLUME_WIDGET(priv->volume_widget));

  g_signal_connect(ido_slider_widget, "style-set", G_CALLBACK(style_changed_cb), NULL);   
  gtk_widget_set_sensitive(ido_slider_widget,
                           !initial_mute);
  gtk_widget_show_all(ido_slider_widget);

  
  GtkMenuItem *menu_volume_item = GTK_MENU_ITEM(ido_slider_widget);
  dbusmenu_gtkclient_newitem_base(DBUSMENU_GTKCLIENT(client),
                                  newitem,
                                  menu_volume_item,
                                  parent);    
  fetch_state(INDICATOR_SOUND (io));      
  return TRUE;  
}


static void
connection_changed (IndicatorServiceManager * sm,
                    gboolean connected,
                    gpointer user_data)
{
  IndicatorSound* indicator = INDICATOR_SOUND(user_data);
  g_return_if_fail ( IS_INDICATOR_SOUND (indicator) );
  IndicatorSoundPrivate* priv = INDICATOR_SOUND_GET_PRIVATE (indicator);
  GError *error = NULL;

  if (connected == FALSE){
    update_state (STATE_SINKS_NONE);
    return;
    //TODO: Gracefully handle disconnection
    // do a timeout to wait for reconnection 
    // for 5 seconds and then if no connection message 
    // is received put the state at 'sink not available'
  }
  // If the proxy is not null and connected is true => its a reconnect,
  // we don't need to anything, gdbus takes care of the rest - bless.
  // just fetch the state.
  if (priv->dbus_proxy != NULL){
    fetch_state (indicator);
    return;
  }
  
  if ( node_info == NULL ){
    node_info = g_dbus_node_info_new_for_xml ( _sound_service,
                                                &error );
    if (error != NULL) {
      g_warning( "Failed to get create interface info from xml: %s",
                 error->message );
      g_error_free(error);
      return;
    }
  }

  if (interface_info == NULL) {
    interface_info = g_dbus_node_info_lookup_interface (node_info,
                                                        INDICATOR_SOUND_DBUS_INTERFACE);
    if (interface_info == NULL) {
      g_error("Unable to find interface '" INDICATOR_SOUND_DBUS_INTERFACE "'");
    }
  }
  
  g_dbus_proxy_new_for_bus( G_BUS_TYPE_SESSION,
                            G_DBUS_PROXY_FLAGS_NONE,
                            interface_info,
                            INDICATOR_SOUND_DBUS_NAME,
                            INDICATOR_SOUND_MENU_DBUS_OBJECT_PATH,
                            INDICATOR_SOUND_DBUS_INTERFACE,
                            NULL,
                            create_connection_to_service,
                            indicator );
}

static void create_connection_to_service (GObject *source_object,
                                          GAsyncResult *res,
                                          gpointer user_data)
{
  IndicatorSound *self = INDICATOR_SOUND(user_data);
  GError *error = NULL;

  g_return_if_fail( IS_INDICATOR_SOUND(self) );

  IndicatorSoundPrivate* priv = INDICATOR_SOUND_GET_PRIVATE(self);

  priv->dbus_proxy = g_dbus_proxy_new_finish(res, &error);

  if (error != NULL) {
    g_warning("Failed to get dbus proxy: %s", error->message);
    g_error_free(error);
    return;
  }

  g_debug ( "about to connect to g-signal ");
  g_signal_connect(priv->dbus_proxy, "g-signal",
                   G_CALLBACK(g_signal_cb), self);
  g_debug ( "after attempting to connect to g-signal ");

  fetch_state (self);
}

static void fetch_state (IndicatorSound* self)
{
  IndicatorSoundPrivate* priv = INDICATOR_SOUND_GET_PRIVATE(self);
  
  g_dbus_proxy_call ( priv->dbus_proxy,
                      "GetSinkMute",
                      NULL,
		                  G_DBUS_CALL_FLAGS_NONE,
                      -1,
                      NULL,
                      (GAsyncReadyCallback)get_sink_mute_cb,
                      self);

  g_dbus_proxy_call ( priv->dbus_proxy,
                      "GetSinkAvailability",
                      NULL,
		                  G_DBUS_CALL_FLAGS_NONE,
                      -1,
                      NULL,
                      (GAsyncReadyCallback)get_sink_availability_cb,
                      self);
}

static void get_sink_availability_cb ( GObject *object,
                                       GAsyncResult *res,
		                                   gpointer user_data )
{
  IndicatorSound* self = INDICATOR_SOUND(user_data);
  IndicatorSoundPrivate* priv = INDICATOR_SOUND_GET_PRIVATE(self);
  
  GVariant *result, *value;
  GError *error = NULL;
  gboolean is_available;

  result = g_dbus_proxy_call_finish ( priv->dbus_proxy,
                                      res,
                                      &error );

  if (error != NULL) {
    g_debug("get_sink_availability call failed: %s", error->message);
    g_error_free(error);
    return;
  }

  value = g_variant_get_child_value(result, 0);
  is_available = g_variant_get_boolean(value);

  if (is_available == FALSE) {
    update_state(STATE_SINKS_NONE);
  }

  if(priv->volume_widget != NULL){
    GtkWidget* slider_widget = volume_widget_get_ido_slider(VOLUME_WIDGET(priv->volume_widget));  
    gtk_widget_set_sensitive(slider_widget, is_available);
  }
    
  g_variant_unref(value);
  g_variant_unref(result);
}


static void get_sink_mute_cb ( GObject *object,
                               GAsyncResult *res,
		                           gpointer user_data)
{
  IndicatorSound* self = INDICATOR_SOUND(user_data);
  IndicatorSoundPrivate* priv = INDICATOR_SOUND_GET_PRIVATE(self);
  
  GVariant *result, *value;
  GError *error = NULL;
  gboolean is_muted;

  result = g_dbus_proxy_call_finish ( priv->dbus_proxy,
                                      res,
                                      &error );

  if (error != NULL) {
    g_debug("get_sink_mute call failed: %s", error->message);
    g_error_free(error);
    return;
  }

  value = g_variant_get_child_value(result, 0);
  is_muted = g_variant_get_boolean(value);

  if ( is_muted == TRUE ){
    update_state(STATE_MUTED);
  }
  
  g_variant_unref(value);
  g_variant_unref(result);
}

/*
Prepare states Array.
*/
void
prepare_state_machine()
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
static void
prepare_blocked_animation()
{
  gchar* blocked_name = g_hash_table_lookup(volume_states, GINT_TO_POINTER(STATE_MUTED_WHILE_INPUT));
  gchar* muted_name = g_hash_table_lookup(volume_states, GINT_TO_POINTER(STATE_MUTED));

  GtkImage* temp_image = indicator_image_helper(muted_name);
  GdkPixbuf* mute_buf = gtk_image_get_pixbuf(temp_image);

  temp_image = indicator_image_helper(blocked_name);
  GdkPixbuf* blocked_buf = gtk_image_get_pixbuf(temp_image);

  if (mute_buf == NULL || blocked_buf == NULL) {
    //g_debug("Don bother with the animation, the theme aint got the goods !");
    return;
  }

  int i;

  // sample 51 snapshots - range : 0-256
  for (i = 0; i < 51; i++) {
    gdk_pixbuf_composite(mute_buf, blocked_buf, 0, 0,
                         gdk_pixbuf_get_width(mute_buf),
                         gdk_pixbuf_get_height(mute_buf),
                         0, 0, 1, 1, GDK_INTERP_BILINEAR, MIN(255, i * 5));
    blocked_animation_list = g_list_append(blocked_animation_list, gdk_pixbuf_copy(blocked_buf));
  }
  g_object_ref_sink(mute_buf);
  g_object_unref(mute_buf);
  g_object_ref_sink(blocked_buf);
  g_object_unref(blocked_buf);
}

gint
get_state()
{
  return current_state;
}

gchar*
get_state_image_name(gint state)
{
  return g_hash_table_lookup(volume_states, GINT_TO_POINTER(state));
}

void
prepare_for_tests(IndicatorObject *io)
{
  prepare_state_machine();
  get_icon(io);
}

void
tidy_up_hash()
{
  g_hash_table_destroy(volume_states);
}

static void
update_state(const gint state)
{
  previous_state = current_state;
  current_state = state;
  gchar* image_name = g_hash_table_lookup(volume_states, GINT_TO_POINTER(current_state));
  indicator_image_helper_update(speaker_image, image_name);
}


void
determine_state_from_volume(gdouble volume_percent)
{
  if (device_available == FALSE)
    return;
  gint state = previous_state;
  if (volume_percent < 30.0 && volume_percent > 0) {
    state = STATE_LOW;
  } else if (volume_percent < 70.0 && volume_percent >= 30.0) {
    state = STATE_MEDIUM;
  } else if (volume_percent >= 70.0) {
    state = STATE_HIGH;
  } else if (volume_percent == 0.0) {
    state = STATE_ZERO;
  }
  update_state(state);
}


static gboolean
start_animation()
{
  blocked_iter = blocked_animation_list;
  blocked_id = 0;
  animation_id = g_timeout_add(50, fade_back_to_mute_image, NULL);
  return FALSE;
}

static gboolean
fade_back_to_mute_image()
{
  if (blocked_iter != NULL) {
    gtk_image_set_from_pixbuf(speaker_image, blocked_iter->data);
    blocked_iter = blocked_iter->next;
    return TRUE;
  } else {
    animation_id = 0;
    //g_debug("exit from animation now\n");
    return FALSE;
  }
}

static void
reset_mute_blocking_animation()
{
  if (animation_id != 0) {
    //g_debug("about to remove the animation_id callback from the mainloop!!**");
    g_source_remove(animation_id);
    animation_id = 0;
  }
  if (blocked_id != 0) {
    //g_debug("about to remove the blocked_id callback from the mainloop!!**");
    g_source_remove(blocked_id);
    blocked_id = 0;
  }
}



/*******************************************************************/
// DBUS Signal reactions
/*******************************************************************/
static void g_signal_cb ( GDBusProxy* proxy,
                          gchar* sender_name,
                          gchar* signal_name,
                          GVariant* parameters,
                          gpointer user_data)
{
  IndicatorSound *self = INDICATOR_SOUND(user_data);
  g_return_if_fail ( IS_INDICATOR_SOUND(self) );

  g_variant_ref (parameters);
  GVariant *value = g_variant_get_child_value (parameters, 0);
  gboolean input = g_variant_get_boolean (value);
  g_variant_unref (parameters);

  g_debug ( "!!! signal_cb with value %i", input);
  
  if (g_strcmp0(signal_name, INDICATOR_SOUND_SIGNAL_SINK_AVAILABLE_UPDATE) == 0){
    react_to_signal_sink_availability_update ( input, self );
  }
  else if (g_strcmp0(signal_name, INDICATOR_SOUND_SIGNAL_SINK_MUTE_UPDATE) == 0){
    react_to_signal_sink_mute_update ( input, self );
  }
  else if (g_strcmp0(signal_name, INDICATOR_SOUND_SIGNAL_SINK_INPUT_WHILE_MUTED) == 0){
    react_to_signal_sink_input_while_muted ( input, self );
  }
}

static void
react_to_signal_sink_input_while_muted(gboolean block_value, IndicatorSound* self)
{
  //g_debug("signal caught - sink input while muted with value %i", block_value);
  if (block_value == 1 && blocked_id == 0 && animation_id == 0 && blocked_animation_list != NULL) {
    gchar* image_name = g_hash_table_lookup(volume_states, GINT_TO_POINTER(STATE_MUTED_WHILE_INPUT));
    indicator_image_helper_update(speaker_image, image_name);
    blocked_id = g_timeout_add_seconds(5, start_animation, NULL);
  }
}

/*
 We can be sure the service won't send a mute signal unless it has changed !
 UNMUTE's force a volume update therefore icon is updated appropriately => no need for unmute handling here.
*/ 
static void
react_to_signal_sink_mute_update(gboolean mute_value, IndicatorSound* self)
{
  if (mute_value == TRUE && device_available == TRUE) {
    update_state(STATE_MUTED);
  } else {
    reset_mute_blocking_animation();
  }
  //g_debug("signal caught - sink mute update with mute value: %i", mute_value);
  IndicatorSoundPrivate* priv = INDICATOR_SOUND_GET_PRIVATE(self);

  if(priv->volume_widget == NULL){
    return;
  }
  GtkWidget* slider_widget = volume_widget_get_ido_slider(VOLUME_WIDGET(priv->volume_widget)); 
  gtk_widget_set_sensitive(slider_widget, !mute_value);
}


static void
react_to_signal_sink_availability_update(gboolean available_value, IndicatorSound* self)
{
  device_available  = available_value;
  if (device_available == FALSE) {
    update_state(STATE_SINKS_NONE);
  }
  //g_debug("signal caught - sink availability update with  value: %i", available_value);
}

/*******************************************************************/
//UI callbacks
/******************************************************************/

/**
key_press_cb:
**/
static gboolean
key_press_cb(GtkWidget* widget, GdkEventKey* event, gpointer data)
{
  gboolean digested = FALSE;

  g_return_val_if_fail(IS_INDICATOR_SOUND(data), FALSE);

  IndicatorSound *indicator = INDICATOR_SOUND (data);

  IndicatorSoundPrivate* priv = INDICATOR_SOUND_GET_PRIVATE(indicator);
  if(priv->volume_widget == NULL){
    return FALSE;
  }
  
  GtkWidget* slider_widget = volume_widget_get_ido_slider(VOLUME_WIDGET(priv->volume_widget)); 
  GtkWidget* slider = ido_scale_menu_item_get_scale((IdoScaleMenuItem*)slider_widget);
  GtkRange* range = (GtkRange*)slider;
  g_return_val_if_fail(GTK_IS_RANGE(range), FALSE);
  gdouble current_value = gtk_range_get_value(range);
  gdouble new_value = current_value;
  const gdouble five_percent = 5;
  GtkWidget *menuitem;

  menuitem = GTK_MENU_SHELL (widget)->active_menu_item;
  if (IDO_IS_SCALE_MENU_ITEM(menuitem) == TRUE) {
    switch (event->keyval) {
    case GDK_Right:
      digested = TRUE;
      if (event->state & GDK_CONTROL_MASK) {
        new_value = 100;
      } else {
        new_value = current_value + five_percent;
      }
      break;
    case GDK_Left:
      digested = TRUE;
      if (event->state & GDK_CONTROL_MASK) {
        new_value = 0;
      } else {
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
    if (new_value != current_value && current_state != STATE_MUTED) {
      //g_debug("Attempting to set the range from the key listener to %f", new_value);
      volume_widget_update(VOLUME_WIDGET(priv->volume_widget), new_value);      
    }
  }
  else if (IS_TRANSPORT_WIDGET(menuitem) == TRUE) {
    TransportWidget* transport_widget = NULL;
    GList* elem;

    for ( elem = priv->transport_widgets_list; elem; elem = elem->next ) {
      transport_widget = TRANSPORT_WIDGET ( elem->data );
      if ( transport_widget_is_selected( transport_widget ) ) 
        break;
    }

    switch (event->keyval) {
    case GDK_Right:
      transport_widget_react_to_key_press_event ( transport_widget,
                                                  TRANSPORT_NEXT );
      digested = TRUE;         
      break;        
    case GDK_Left:
      transport_widget_react_to_key_press_event ( transport_widget,
                                                  TRANSPORT_PREVIOUS );
      digested = TRUE;         
      break;                  
    case GDK_KEY_space:
      transport_widget_react_to_key_press_event ( transport_widget,
                                                  TRANSPORT_PLAY_PAUSE );        
      digested = TRUE;         
      break;
    case GDK_Up:
    case GDK_Down:
      digested = FALSE;     
      break;
    default:
      break;
    }
  } 
  return digested;
}


/**
key_release_cb:
**/
static gboolean
key_release_cb(GtkWidget* widget, GdkEventKey* event, gpointer data)
{
  gboolean digested = FALSE;

  g_return_val_if_fail(IS_INDICATOR_SOUND(data), FALSE);

  IndicatorSound *indicator = INDICATOR_SOUND (data);

  IndicatorSoundPrivate* priv = INDICATOR_SOUND_GET_PRIVATE(indicator);

  GtkWidget *menuitem;

  menuitem = GTK_MENU_SHELL (widget)->active_menu_item;
  if (IS_TRANSPORT_WIDGET(menuitem) == TRUE) {
    TransportWidget* transport_widget = NULL;
    GList* elem;

    for(elem = priv->transport_widgets_list; elem; elem = elem->next) {
      transport_widget = TRANSPORT_WIDGET (elem->data);
      if ( transport_widget_is_selected( transport_widget ) ) 
        break;
    }

    switch (event->keyval) {
    case GDK_Right:
      transport_widget_react_to_key_release_event ( transport_widget,
                                                    TRANSPORT_NEXT );
      digested = TRUE;
      break;        
    case GDK_Left:
      transport_widget_react_to_key_release_event ( transport_widget,
                                                    TRANSPORT_PREVIOUS );
      digested = TRUE;         
      break;                  
    case GDK_KEY_space:
      transport_widget_react_to_key_release_event ( transport_widget,
                                                    TRANSPORT_PLAY_PAUSE );        
      digested = TRUE;         
      break;
    case GDK_Up:
    case GDK_Down:
      digested = FALSE;     
      break;
    default:
      break;
    }
  } 
  return digested;
}

static void
style_changed_cb(GtkWidget *widget, gpointer user_data)
{
  //g_debug("Just caught a style change event");
  update_state(current_state);
  reset_mute_blocking_animation();
  update_state(current_state);
  free_the_animation_list();
  prepare_blocked_animation();
}

static void
indicator_sound_scroll (IndicatorObject *io, gint delta, IndicatorScrollDirection direction)
{
  //g_debug("indicator-sound-scroll - current slider value");

  if (device_available == FALSE || current_state == STATE_MUTED)
    return;
  IndicatorSoundPrivate* priv = INDICATOR_SOUND_GET_PRIVATE(INDICATOR_SOUND (io));
  
  GtkWidget* slider_widget = volume_widget_get_ido_slider(VOLUME_WIDGET(priv->volume_widget)); 
  GtkWidget* slider = ido_scale_menu_item_get_scale((IdoScaleMenuItem*)slider_widget);
  GtkRange* range = (GtkRange*)slider;
  g_return_if_fail(GTK_IS_RANGE(range));

  gdouble value = gtk_range_get_value(range);
  GtkAdjustment *adj = gtk_range_get_adjustment (GTK_RANGE (slider));
  //g_debug("indicator-sound-scroll - current slider value %f", value);
  if (direction == INDICATOR_OBJECT_SCROLL_UP) {
    value += adj->step_increment;
  } else {
    value -= adj->step_increment;
  }
  //g_debug("indicator-sound-scroll - update slider with value %f", value);
  volume_widget_update(VOLUME_WIDGET(priv->volume_widget), value);  
}
