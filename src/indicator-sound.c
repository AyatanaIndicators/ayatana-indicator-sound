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

#include "config.h"

#include <math.h>
#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <libdbusmenu-gtk/menu.h>
#include <libido/idoscalemenuitem.h>

#include <gio/gio.h>

#include "indicator-sound.h"
#include "transport-widget.h"
#include "metadata-widget.h"
#include "volume-widget.h"
#include "voip-input-widget.h"
#include "dbus-shared-names.h"
#include "sound-state-manager.h"
#include "mute-widget.h"

#include "gen-sound-service.xml.h"
#include "common-defs.h"

typedef struct _IndicatorSoundPrivate IndicatorSoundPrivate;

struct _IndicatorSoundPrivate
{
  GtkWidget* volume_widget;
  GtkWidget* voip_widget;
  MuteWidget *mute_widget;
  GList* transport_widgets_list;
  GDBusProxy *dbus_proxy; 
  SoundStateManager* state_manager;
  gchar *accessible_desc;
  GSettings *settings;
};

#define INDICATOR_SOUND_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), INDICATOR_SOUND_TYPE, IndicatorSoundPrivate))

#define SOUND_INDICATOR_GSETTINGS_SCHEMA_ID "com.canonical.indicator.sound"

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
static const gchar * get_accessible_desc (IndicatorObject * io);
static const gchar * get_name_hint (IndicatorObject * io);
static void indicator_sound_scroll (IndicatorObject * io,
                                    IndicatorObjectEntry * entry, gint delta,
                                    IndicatorScrollDirection direction);
static void indicator_sound_middle_click (IndicatorObject * io,
                                          IndicatorObjectEntry * entry,
                                          guint time, gpointer data);

//key/moust event handlers
static gboolean key_press_cb(GtkWidget* widget, GdkEventKey* event, gpointer data);
static gboolean key_release_cb(GtkWidget* widget, GdkEventKey* event, gpointer data);

//custom widget realisation methods
static gboolean new_volume_slider_widget (DbusmenuMenuitem * newitem,
                                          DbusmenuMenuitem * parent,
                                          DbusmenuClient * client,
                                          gpointer user_data);
static gboolean new_voip_slider_widget (DbusmenuMenuitem * newitem,
                                          DbusmenuMenuitem * parent,
                                          DbusmenuClient * client,
                                          gpointer user_data);
static gboolean new_transport_widget (DbusmenuMenuitem * newitem,
                                      DbusmenuMenuitem * parent,
                                      DbusmenuClient * client,
                                      gpointer user_data);                                     
static gboolean new_metadata_widget (DbusmenuMenuitem * newitem,
                                     DbusmenuMenuitem * parent,
                                     DbusmenuClient * client,
                                     gpointer user_data);
static gboolean new_mute_widget (DbusmenuMenuitem * newitem,
                                     DbusmenuMenuitem * parent,
                                     DbusmenuClient * client,
                                     gpointer user_data);

// DBUS communication
static GDBusNodeInfo *node_info = NULL;
static GDBusInterfaceInfo *interface_info = NULL;
static void create_connection_to_service (GObject *source_object,
                                          GAsyncResult *res,
                                          gpointer user_data);
static void connection_changed (IndicatorServiceManager * sm,
                                gboolean connected,
                                gpointer userdata);

// Visiblity
static void settings_init (IndicatorSound * self);


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
  io_class->get_accessible_desc = get_accessible_desc;
  io_class->get_name_hint = get_name_hint;
  io_class->entry_scrolled = indicator_sound_scroll;
  io_class->secondary_activate = indicator_sound_middle_click;
}

static void
indicator_sound_init (IndicatorSound *self)
{
  self->service = NULL;
  self->service = indicator_service_manager_new_version(INDICATOR_SOUND_DBUS_NAME,
                                                        INDICATOR_SOUND_DBUS_VERSION);
  
  IndicatorSoundPrivate* priv = INDICATOR_SOUND_GET_PRIVATE(self);
  priv->volume_widget = NULL;
  priv->voip_widget = NULL;
  priv->mute_widget = NULL;
  priv->dbus_proxy = NULL;
  GList* t_list = NULL;
  priv->transport_widgets_list = t_list;
  priv->state_manager = g_object_new (SOUND_TYPE_STATE_MANAGER, NULL);
  priv->accessible_desc = NULL;
  priv->settings = NULL;

  settings_init (self);

  g_signal_connect ( G_OBJECT(self->service),
                     INDICATOR_SERVICE_MANAGER_SIGNAL_CONNECTION_CHANGE,
                     G_CALLBACK(connection_changed), self );
}

static void
indicator_sound_dispose (GObject *object)
{
  IndicatorSound * self = INDICATOR_SOUND(object);
  IndicatorSoundPrivate* priv = INDICATOR_SOUND_GET_PRIVATE(self);

  if (priv->settings != NULL) {
    g_object_unref (G_OBJECT(priv->settings));
    priv->settings = NULL;
  }

  if (self->service != NULL) {
    g_object_unref(G_OBJECT(self->service));
    self->service = NULL;
  }
  g_list_free (priv->transport_widgets_list);

  G_OBJECT_CLASS (indicator_sound_parent_class)->dispose (object);
}

static void
indicator_sound_finalize (GObject *object)
{
  IndicatorSound * self = INDICATOR_SOUND(object);
  IndicatorSoundPrivate* priv = INDICATOR_SOUND_GET_PRIVATE(self);

  if (priv->accessible_desc) {
    g_free (priv->accessible_desc);
    priv->accessible_desc = NULL;
  }

  G_OBJECT_CLASS (indicator_sound_parent_class)->finalize (object);
}

static GtkLabel *
get_label (IndicatorObject * io)
{
  return NULL;
}

static GtkImage *
get_icon (IndicatorObject * io)
{
  IndicatorSoundPrivate* priv = INDICATOR_SOUND_GET_PRIVATE(INDICATOR_SOUND (io));
  gtk_widget_show( GTK_WIDGET(sound_state_manager_get_current_icon (priv->state_manager)) );  
  return sound_state_manager_get_current_icon (priv->state_manager);
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
  dbusmenu_client_add_type_handler (DBUSMENU_CLIENT(client),
                                    DBUSMENU_VOLUME_MENUITEM_TYPE,
                                    new_volume_slider_widget);
  dbusmenu_client_add_type_handler (DBUSMENU_CLIENT(client),
                                    DBUSMENU_VOIP_INPUT_MENUITEM_TYPE,
                                    new_voip_slider_widget);
  dbusmenu_client_add_type_handler (DBUSMENU_CLIENT(client),
                                    DBUSMENU_TRANSPORT_MENUITEM_TYPE,
                                    new_transport_widget);
  dbusmenu_client_add_type_handler (DBUSMENU_CLIENT(client),
                                    DBUSMENU_METADATA_MENUITEM_TYPE,
                                    new_metadata_widget);
  dbusmenu_client_add_type_handler (DBUSMENU_CLIENT(client),
                                    DBUSMENU_MUTE_MENUITEM_TYPE,
                                    new_mute_widget);
  // Note: Not ideal but all key handling needs to be managed here and then 
  // delegated to the appropriate widget. 
  g_signal_connect (menu, "key-press-event", G_CALLBACK(key_press_cb), io);
  g_signal_connect (menu, "key-release-event", G_CALLBACK(key_release_cb), io);
  
  return GTK_MENU(menu);
}

static const gchar *
get_accessible_desc (IndicatorObject * io)
{
  IndicatorSoundPrivate* priv = INDICATOR_SOUND_GET_PRIVATE(io);
  return priv->accessible_desc;
}

static const gchar *get_name_hint (IndicatorObject * io)
{
  return PACKAGE_NAME;
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
    sound_state_manager_deal_with_disconnect (priv->state_manager);
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
    g_dbus_proxy_call ( priv->dbus_proxy,
                        "GetSoundState",
                        NULL,
		                    G_DBUS_CALL_FLAGS_NONE,
                        -1,
                        NULL,
                        (GAsyncReadyCallback)sound_state_manager_get_state_cb,
                        priv->state_manager);  
    return;
  }
  
  if ( node_info == NULL ){
    node_info = g_dbus_node_info_new_for_xml ( _sound_service,
                                                &error );
    if (error != NULL) {
      g_critical ( "Failed to get create interface info from xml: %s",
                  error->message );
      g_error_free(error);
      return;
    }
  }

  if (interface_info == NULL) {
    interface_info = g_dbus_node_info_lookup_interface (node_info,
                                                        INDICATOR_SOUND_DBUS_INTERFACE);
    if (interface_info == NULL) {
      g_critical ("Unable to find interface '" INDICATOR_SOUND_DBUS_INTERFACE "'");
    }
  }
  
  g_dbus_proxy_new_for_bus( G_BUS_TYPE_SESSION,
                            G_DBUS_PROXY_FLAGS_NONE,
                            interface_info,
                            INDICATOR_SOUND_DBUS_NAME,
                            INDICATOR_SOUND_SERVICE_DBUS_OBJECT_PATH,
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
    g_critical ("Failed to get dbus proxy: %s", error->message);
    g_error_free(error);
    return;
  }
  sound_state_manager_connect_to_dbus (priv->state_manager,
                                       priv->dbus_proxy);
  
}

static gboolean
new_transport_widget (DbusmenuMenuitem * newitem,
                      DbusmenuMenuitem * parent,
                      DbusmenuClient * client,
                      gpointer user_data)
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
  dbusmenu_gtkclient_newitem_base (DBUSMENU_GTKCLIENT(client),
                                   newitem,
                                   menu_transport_bar,
                                   parent);
  return TRUE;
}

static gboolean
new_metadata_widget (DbusmenuMenuitem * newitem,
                     DbusmenuMenuitem * parent,
                     DbusmenuClient * client,
                     gpointer user_data)
{
  g_debug("indicator-sound: new_metadata_widget");

  GtkWidget* metadata = NULL;

  g_return_val_if_fail(DBUSMENU_IS_MENUITEM(newitem), FALSE);
  g_return_val_if_fail(DBUSMENU_IS_GTKCLIENT(client), FALSE);


  metadata = metadata_widget_new (newitem);
  
  g_debug ("%s (\"%s\")", __func__,
           dbusmenu_menuitem_property_get(newitem, DBUSMENU_METADATA_MENUITEM_PLAYER_NAME));
  
  GtkMenuItem *menu_metadata_widget = GTK_MENU_ITEM(metadata);

  gtk_widget_show_all(metadata);
  dbusmenu_gtkclient_newitem_base (DBUSMENU_GTKCLIENT(client),
                                   newitem,
                                   menu_metadata_widget,
                                   parent);
  return TRUE;
}

static gboolean
new_volume_slider_widget(DbusmenuMenuitem * newitem,
                         DbusmenuMenuitem * parent,
                         DbusmenuClient * client,
                         gpointer user_data)
{
  g_debug("indicator-sound: new_volume_slider_widget");

  GtkWidget* volume_widget = NULL;
  IndicatorObject *io = NULL;

  g_return_val_if_fail(DBUSMENU_IS_MENUITEM(newitem), FALSE);
  g_return_val_if_fail(DBUSMENU_IS_GTKCLIENT(client), FALSE);

  io = g_object_get_data (G_OBJECT (client), "indicator");
  IndicatorSoundPrivate* priv = INDICATOR_SOUND_GET_PRIVATE(INDICATOR_SOUND (io));

  if (priv->volume_widget != NULL){ 
    volume_widget_tidy_up (priv->volume_widget);
    gtk_widget_destroy (priv->volume_widget);
    priv->volume_widget = NULL;
  }
  volume_widget = volume_widget_new (newitem, io);
  priv->volume_widget = volume_widget;
  // Don't forget to set the accessible desc.
  update_accessible_desc (io);
  

  GtkWidget* ido_slider_widget = volume_widget_get_ido_slider(VOLUME_WIDGET(priv->volume_widget));

  gtk_widget_show_all(ido_slider_widget);
  // register the style callback on this widget with state manager's style change
  // handler (needs to remake the blocking animation for each style).
  g_signal_connect (ido_slider_widget, "style-set",
                    G_CALLBACK(sound_state_manager_style_changed_cb),
                    priv->state_manager);     

  GtkMenuItem *menu_volume_item = GTK_MENU_ITEM(ido_slider_widget);
  dbusmenu_gtkclient_newitem_base(DBUSMENU_GTKCLIENT(client),
                                  newitem,
                                  menu_volume_item,
                                  parent);
  return TRUE;
}
/**
 * new_voip_slider_widget
 * Create the voip menu item widget, must of the time this widget will be hidden.
 * @param newitem
 * @param parent
 * @param client
 * @param user_data
 * @return
 */
static gboolean
new_voip_slider_widget (DbusmenuMenuitem * newitem,
                        DbusmenuMenuitem * parent,
                        DbusmenuClient * client,
                        gpointer user_data)
{
  g_debug("indicator-sound: new_voip_slider_widget");
  GtkWidget* voip_widget = NULL;
  IndicatorObject *io = NULL;

  g_return_val_if_fail(DBUSMENU_IS_MENUITEM(newitem), FALSE);
  g_return_val_if_fail(DBUSMENU_IS_GTKCLIENT(client), FALSE);

  io = g_object_get_data (G_OBJECT (client), "indicator");
  IndicatorSoundPrivate* priv = INDICATOR_SOUND_GET_PRIVATE(INDICATOR_SOUND (io));

  if (priv->voip_widget != NULL){ 
    voip_input_widget_tidy_up (priv->voip_widget);
    gtk_widget_destroy (priv->voip_widget);
    priv->voip_widget = NULL;
  }

  voip_widget = voip_input_widget_new (newitem);
  priv->voip_widget = voip_widget;

  GtkWidget* ido_slider_widget = voip_input_widget_get_ido_slider(VOIP_INPUT_WIDGET(voip_widget));

  gtk_widget_show_all(ido_slider_widget);

  GtkMenuItem *menu_volume_item = GTK_MENU_ITEM(ido_slider_widget);
  dbusmenu_gtkclient_newitem_base(DBUSMENU_GTKCLIENT(client),
                                  newitem,
                                  menu_volume_item,
                                  parent);
  return TRUE;
}

static gboolean
new_mute_widget(DbusmenuMenuitem * newitem,
                DbusmenuMenuitem * parent,
                DbusmenuClient * client,
                gpointer user_data)
{
  IndicatorObject *io = NULL;

  g_return_val_if_fail(DBUSMENU_IS_MENUITEM(newitem), FALSE);
  g_return_val_if_fail(DBUSMENU_IS_GTKCLIENT(client), FALSE);

  io = g_object_get_data (G_OBJECT (client), "indicator");
  IndicatorSoundPrivate* priv = INDICATOR_SOUND_GET_PRIVATE(INDICATOR_SOUND (io));
  
  if (priv->mute_widget != NULL){ 
    g_object_unref (priv->mute_widget);
    priv->mute_widget = NULL;
  }

  priv->mute_widget = mute_widget_new(newitem);
  GtkMenuItem *item = mute_widget_get_menu_item (priv->mute_widget);

  dbusmenu_gtkclient_newitem_base(DBUSMENU_GTKCLIENT(client),
                                  newitem,
                                  item,
                                  parent);

  return TRUE;
}

/*******************************************************************/
//UI callbacks
/******************************************************************/

static GtkWidget *
get_current_item (GtkContainer * container)
{
  GList *children = gtk_container_get_children (container);
  GList *iter;
  GtkWidget *rv = NULL;

  /* Suprisingly, GTK+ doesn't really let us query "what is the currently
     selected item?".  But it does note it internally by prelighting the
     widget, so we watch for that. */
  for (iter = children; iter; iter = iter->next) {
    if (gtk_widget_get_state (GTK_WIDGET (iter->data)) & GTK_STATE_PRELIGHT) {
      rv = GTK_WIDGET (iter->data);
      break;
    }
  }

  return rv;
}

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
  GtkWidget *menuitem;
  menuitem = get_current_item (GTK_CONTAINER (widget));

  if (IDO_IS_SCALE_MENU_ITEM(menuitem) == TRUE){
    gdouble current_value = 0;
    gdouble new_value = 0;
    const gdouble five_percent = 5;
    gboolean is_voip_slider = FALSE;

    if (g_ascii_strcasecmp (ido_scale_menu_item_get_primary_label (IDO_SCALE_MENU_ITEM(menuitem)), "VOLUME") == 0) {
      g_debug ("vOLUME SLIDER KEY PRESS");
      GtkWidget* slider_widget = volume_widget_get_ido_slider(VOLUME_WIDGET(priv->volume_widget));
      GtkWidget* slider = ido_scale_menu_item_get_scale((IdoScaleMenuItem*)slider_widget);
      GtkRange* range = (GtkRange*)slider;
      g_return_val_if_fail(GTK_IS_RANGE(range), FALSE);
      current_value = gtk_range_get_value(range);
      new_value = current_value;
    }
    else if (g_ascii_strcasecmp (ido_scale_menu_item_get_primary_label (IDO_SCALE_MENU_ITEM(menuitem)), "VOIP") == 0) {
      g_debug ("VOIP SLIDER KEY PRESS");
      GtkWidget* slider_widget = voip_input_widget_get_ido_slider(VOIP_INPUT_WIDGET(priv->voip_widget));
      GtkWidget* slider = ido_scale_menu_item_get_scale((IdoScaleMenuItem*)slider_widget);
      GtkRange* range = (GtkRange*)slider;
      g_return_val_if_fail(GTK_IS_RANGE(range), FALSE);
      current_value = gtk_range_get_value(range);
      new_value = current_value;
      is_voip_slider = TRUE;
    }

    switch (event->keyval) {
    case GDK_KEY_Right:
      digested = TRUE;
      new_value = current_value + five_percent;
      break;
    case GDK_KEY_Left:
      digested = TRUE;
      new_value = current_value - five_percent;
      break;
    case GDK_KEY_plus:
      digested = TRUE;
      new_value = current_value + five_percent;
      break;
    case GDK_KEY_minus:
      digested = TRUE;
      new_value = current_value - five_percent;
      break;
    default:
      break;
    }
    new_value = CLAMP(new_value, 0, 100);
    if (new_value != current_value){
      if (is_voip_slider == TRUE){
        voip_input_widget_update (VOIP_INPUT_WIDGET(priv->voip_widget), new_value);
      }
      else{
        volume_widget_update (VOLUME_WIDGET(priv->volume_widget), new_value, "keypress-update");
      }
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
    case GDK_KEY_Right:
      transport_widget_react_to_key_press_event ( transport_widget,
                                                  TRANSPORT_ACTION_NEXT );
      digested = TRUE;         
      break;        
    case GDK_KEY_Left:
      transport_widget_react_to_key_press_event ( transport_widget,
                                                  TRANSPORT_ACTION_PREVIOUS );
      digested = TRUE;         
      break;                  
    case GDK_KEY_space:
      transport_widget_react_to_key_press_event ( transport_widget,
                                                  TRANSPORT_ACTION_PLAY_PAUSE );
      digested = TRUE;         
      break;
    case GDK_KEY_Up:
    case GDK_KEY_Down:
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

  menuitem = get_current_item (GTK_CONTAINER (widget));
  if (IS_TRANSPORT_WIDGET(menuitem) == TRUE) {
    TransportWidget* transport_widget = NULL;
    GList* elem;

    for(elem = priv->transport_widgets_list; elem; elem = elem->next) {
      transport_widget = TRANSPORT_WIDGET (elem->data);
      if ( transport_widget_is_selected( transport_widget ) ) 
        break;
    }

    switch (event->keyval) {
    case GDK_KEY_Right:
      transport_widget_react_to_key_release_event ( transport_widget,
                                                    TRANSPORT_ACTION_NEXT );
      digested = TRUE;
      break;        
    case GDK_KEY_Left:
      transport_widget_react_to_key_release_event ( transport_widget,
                                                    TRANSPORT_ACTION_PREVIOUS );
      digested = TRUE;         
      break;                  
    case GDK_KEY_space:
      transport_widget_react_to_key_release_event ( transport_widget,
                                                    TRANSPORT_ACTION_PLAY_PAUSE );
      digested = TRUE;         
      break;
    case GDK_KEY_Up:
    case GDK_KEY_Down:
      digested = FALSE;     
      break;
    default:
      break;
    }
  } 
  return digested;
}

static void
indicator_sound_scroll (IndicatorObject * io, IndicatorObjectEntry * entry,
                        gint delta, IndicatorScrollDirection direction)
{
  //g_debug("indicator-sound-scroll - current slider value");
  IndicatorSoundPrivate* priv = INDICATOR_SOUND_GET_PRIVATE(INDICATOR_SOUND (io));
  SoundState current_state = sound_state_manager_get_current_state (priv->state_manager);

  if (current_state == UNAVAILABLE || current_state == MUTED)
    return;
  
  GtkWidget* slider_widget = volume_widget_get_ido_slider(VOLUME_WIDGET(priv->volume_widget)); 
  GtkWidget* slider = ido_scale_menu_item_get_scale((IdoScaleMenuItem*)slider_widget);
  GtkRange* range = (GtkRange*)slider;
  g_return_if_fail(GTK_IS_RANGE(range));

  gdouble value = gtk_range_get_value(range);
  GtkAdjustment *adj = gtk_range_get_adjustment (GTK_RANGE (slider));
  //g_debug("indicator-sound-scroll - current slider value %f", value);
  if (direction == INDICATOR_OBJECT_SCROLL_UP) {
    value += gtk_adjustment_get_step_increment (adj);
  } else {
    value -= gtk_adjustment_get_step_increment (adj);
  }
  //g_debug("indicator-sound-scroll - update slider with value %f", value);
  volume_widget_update(VOLUME_WIDGET(priv->volume_widget), value, "scroll updates");

  if (!gtk_widget_get_mapped(GTK_WIDGET (entry->menu)))
    sound_state_manager_show_notification (priv->state_manager, value);
}

static void
indicator_sound_middle_click (IndicatorObject * io, IndicatorObjectEntry * entry,
                              guint time, gpointer data)
{
  IndicatorSoundPrivate* priv = INDICATOR_SOUND_GET_PRIVATE(io);
  g_return_if_fail (priv);

  mute_widget_toggle(priv->mute_widget);
}

void
update_accessible_desc (IndicatorObject * io)
{
  GList *entries = indicator_object_get_entries(io);
  if (!entries)
    return;
  IndicatorObjectEntry * entry = (IndicatorObjectEntry *)entries->data;

  IndicatorSoundPrivate* priv = INDICATOR_SOUND_GET_PRIVATE(io);
  gchar *old_desc = priv->accessible_desc;

  if (priv->volume_widget) {
    priv->accessible_desc = g_strdup_printf(_("Volume (%'.0f%%)"),
                                            volume_widget_get_current_volume (priv->volume_widget));
  }
  else {
    priv->accessible_desc = NULL;
  }

  entry->accessible_desc = priv->accessible_desc;
  g_free (old_desc); 
  g_signal_emit(G_OBJECT(io),
                INDICATOR_OBJECT_SIGNAL_ACCESSIBLE_DESC_UPDATE_ID,
                0,
                entry,
                TRUE);
  g_list_free(entries);
}

/***
****
***/

#define VISIBLE_KEY "visible"

static void
on_visible_changed (GSettings * settings, gchar * key, gpointer user_data)
{
  g_return_if_fail (!g_strcmp0 (key, VISIBLE_KEY));

  IndicatorObject * io = INDICATOR_OBJECT(user_data);
  const gboolean visible = g_settings_get_boolean (settings, key);
  indicator_object_set_visible (io, visible);
  if (visible)
    update_accessible_desc (io); // requires an entry
}

static void
settings_init (IndicatorSound *self)
{
  const char * schema = SOUND_INDICATOR_GSETTINGS_SCHEMA_ID;

  gint i;
  gboolean schema_exists = FALSE;
  const char * const * schemas = g_settings_list_schemas ();
  for (i=0; !schema_exists && schemas && schemas[i]; i++)
    if (!g_strcmp0 (schema, schemas[i]))
      schema_exists = TRUE;

  IndicatorSoundPrivate* priv = INDICATOR_SOUND_GET_PRIVATE(self);
  if (schema_exists) {
    priv->settings = g_settings_new (schema);
  } else {
    priv->settings = NULL;
  }

  if (priv->settings != NULL) {
    g_signal_connect (G_OBJECT(priv->settings), "changed::" VISIBLE_KEY,
                      G_CALLBACK(on_visible_changed), self);
    const gboolean b = g_settings_get_boolean (priv->settings, VISIBLE_KEY);
    g_object_set (G_OBJECT(self),
                  "indicator-object-default-visibility", b,
                  NULL);
  }
}
