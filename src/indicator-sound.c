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

#include <math.h>
#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n.h>
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
#include "voip-input-widget.h"
#include "dbus-shared-names.h"
#include "sound-state-manager.h"

#include "gen-sound-service.xml.h"
#include "common-defs.h"

typedef struct _IndicatorSoundPrivate IndicatorSoundPrivate;

struct _IndicatorSoundPrivate
{
  GtkWidget* volume_widget;
  GList* transport_widgets_list;
  GDBusProxy *dbus_proxy; 
  SoundStateManager* state_manager;
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
static const gchar * get_accessible_desc (IndicatorObject * io);
static void indicator_sound_scroll (IndicatorObject* io,
                                    gint delta,
                                    IndicatorScrollDirection direction);

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
static gboolean new_title_widget (DbusmenuMenuitem * newitem,
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
  io_class->scroll    = indicator_sound_scroll;
}

static void
indicator_sound_init (IndicatorSound *self)
{
  self->service = NULL;
  self->service = indicator_service_manager_new_version(INDICATOR_SOUND_DBUS_NAME,
                                                        INDICATOR_SOUND_DBUS_VERSION);
  
  IndicatorSoundPrivate* priv = INDICATOR_SOUND_GET_PRIVATE(self);
  priv->volume_widget = NULL;
  priv->dbus_proxy = NULL;
  GList* t_list = NULL;
  priv->transport_widgets_list = t_list;
  priv->state_manager = g_object_new (SOUND_TYPE_STATE_MANAGER, NULL);

  g_signal_connect ( G_OBJECT(self->service),
                     INDICATOR_SERVICE_MANAGER_SIGNAL_CONNECTION_CHANGE,
                     G_CALLBACK(connection_changed), self );
}

static void
indicator_sound_dispose (GObject *object)
{
  IndicatorSound * self = INDICATOR_SOUND(object);
  IndicatorSoundPrivate* priv = INDICATOR_SOUND_GET_PRIVATE(self);

  if (self->service != NULL) {
    g_object_unref(G_OBJECT(self->service));
    self->service = NULL;
  }
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
                                    DBUSMENU_TITLE_MENUITEM_TYPE,
                                    new_title_widget);
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

  if (priv->volume_widget != NULL){
    return g_strdup_printf(_("Volume (%'.0f%%)"), volume_widget_get_current_volume(priv->volume_widget));
  }

  return NULL;
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
    g_warning("Failed to get dbus proxy: %s", error->message);
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
  GtkMenuItem *menu_metadata_widget = GTK_MENU_ITEM(metadata);

  gtk_widget_show_all(metadata);
  dbusmenu_gtkclient_newitem_base (DBUSMENU_GTKCLIENT(client),
                                   newitem, menu_metadata_widget, parent);
  return TRUE;
}

static gboolean
new_title_widget(DbusmenuMenuitem * newitem,
                 DbusmenuMenuitem * parent,
                 DbusmenuClient * client,
                 gpointer user_data)
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
  volume_widget = volume_widget_new (newitem, io);
  IndicatorSoundPrivate* priv = INDICATOR_SOUND_GET_PRIVATE(INDICATOR_SOUND (io));
  priv->volume_widget = volume_widget;

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

  g_return_val_if_fail(DBUSMENU_IS_MENUITEM(newitem), FALSE);
  g_return_val_if_fail(DBUSMENU_IS_GTKCLIENT(client), FALSE);

  voip_widget = voip_input_widget_new (newitem);

  GtkWidget* ido_slider_widget = voip_input_widget_get_ido_slider(VOIP_INPUT_WIDGET(voip_widget));

  gtk_widget_show_all(ido_slider_widget);

  GtkMenuItem *menu_volume_item = GTK_MENU_ITEM(ido_slider_widget);
  dbusmenu_gtkclient_newitem_base(DBUSMENU_GTKCLIENT(client),
                                  newitem,
                                  menu_volume_item,
                                  parent);
  return TRUE;
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
      new_value = current_value + five_percent;
      break;
    case GDK_Left:
      digested = TRUE;
      new_value = current_value - five_percent;
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
    if (new_value != current_value && sound_state_manager_get_current_state (priv->state_manager) != MUTED) {
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
                                                  TRANSPORT_ACTION_NEXT );
      digested = TRUE;         
      break;        
    case GDK_Left:
      transport_widget_react_to_key_press_event ( transport_widget,
                                                  TRANSPORT_ACTION_PREVIOUS );
      digested = TRUE;         
      break;                  
    case GDK_KEY_space:
      transport_widget_react_to_key_press_event ( transport_widget,
                                                  TRANSPORT_ACTION_PLAY_PAUSE );
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
                                                    TRANSPORT_ACTION_NEXT );
      digested = TRUE;
      break;        
    case GDK_Left:
      transport_widget_react_to_key_release_event ( transport_widget,
                                                    TRANSPORT_ACTION_PREVIOUS );
      digested = TRUE;         
      break;                  
    case GDK_KEY_space:
      transport_widget_react_to_key_release_event ( transport_widget,
                                                    TRANSPORT_ACTION_PLAY_PAUSE );
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
indicator_sound_scroll (IndicatorObject *io, gint delta, 
                        IndicatorScrollDirection direction)
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
    value += adj->step_increment;
  } else {
    value -= adj->step_increment;
  }
  //g_debug("indicator-sound-scroll - update slider with value %f", value);
  volume_widget_update(VOLUME_WIDGET(priv->volume_widget), value);

  sound_state_manager_show_notification (priv->state_manager, value);
}

void
update_accessible_desc (IndicatorObject * io)
{
  GList *entries = indicator_object_get_entries(io);
  IndicatorObjectEntry * entry = (IndicatorObjectEntry *)entries->data;
  entry->accessible_desc = get_accessible_desc(io);
  g_signal_emit(G_OBJECT(io),
                INDICATOR_OBJECT_SIGNAL_ACCESSIBLE_DESC_UPDATE_ID,
                0,
                entry,
                TRUE);
  g_list_free(entries);
}
