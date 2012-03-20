/*
 * Copyright 2010 Canonical Ltd.
 *
 * Authors:
 *     Conor Curran <conor.curran@canonical.com>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gio/gio.h>
#include <unistd.h>
#include <glib/gi18n.h>
#include <libindicator/indicator-service.h>
#include <libdbusmenu-glib/server.h>
#include <libdbusmenu-glib/client.h>

#include "sound-service-dbus.h"
#include "device.h"
#include "gen-sound-service.xml.h"
#include "dbus-shared-names.h"
#include "sound-service-marshal.h"

// DBUS methods
static void bus_method_call (GDBusConnection * connection,
                             const gchar * sender,
                             const gchar * path,
                             const gchar * interface,
                             const gchar * method,
                             GVariant * params, 
                             GDBusMethodInvocation * invocation,
                             gpointer user_data);

static GDBusInterfaceVTable       interface_table = {
	method_call:	bus_method_call,
	get_property:	NULL, /* No properties */
	set_property:	NULL  /* No properties */
};


typedef struct _SoundServiceDbusPrivate SoundServiceDbusPrivate;

struct _SoundServiceDbusPrivate {
  GDBusConnection*    connection;
  DbusmenuMenuitem*   root_menuitem;
  Device*             device;
  gboolean            greeter_mode;
  guint               registration_id;
};

enum {
  TRACK_SPECIFIC_ITEM,
  PLAYER_SPECIFIC_ITEM,  
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

static GDBusNodeInfo *            node_info = NULL;
static GDBusInterfaceInfo *       interface_info = NULL;

#define SOUND_SERVICE_DBUS_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), SOUND_SERVICE_DBUS_TYPE, SoundServiceDbusPrivate))

static void sound_service_dbus_class_init (SoundServiceDbusClass *klass);
static void sound_service_dbus_init       (SoundServiceDbus *self);
static void sound_service_dbus_dispose    (GObject *object);
static void sound_service_dbus_finalize   (GObject *object);

static void show_sound_settings_dialog (DbusmenuMenuitem *mi,
                                        gpointer user_data);
static gboolean sound_service_dbus_blacklist_player (SoundServiceDbus* self,
                                                     const gchar* player_name,
                                                     gboolean blacklist); 

static gboolean sound_service_dbus_is_blacklisted (SoundServiceDbus* self,
                                                   const gchar* player_name);

G_DEFINE_TYPE (SoundServiceDbus, sound_service_dbus, G_TYPE_OBJECT);

static void
sound_service_dbus_class_init (SoundServiceDbusClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (object_class, sizeof(SoundServiceDbusPrivate));

  object_class->dispose = sound_service_dbus_dispose;
  object_class->finalize = sound_service_dbus_finalize;

  g_assert(klass != NULL);

  if (node_info == NULL) {
    GError * error = NULL;

    node_info = g_dbus_node_info_new_for_xml(_sound_service, &error);
    if (error != NULL) {
      g_critical ("Unable to parse Indicator Service Interface description: %s",
               error->message);
      g_error_free(error);
    }
  }

  if (interface_info == NULL) {
    interface_info = g_dbus_node_info_lookup_interface (node_info,
                                                        INDICATOR_SOUND_DBUS_INTERFACE);

    if (interface_info == NULL) {
      g_critical("Unable to find interface '" INDICATOR_SOUND_DBUS_INTERFACE "'");
    }
  }
  signals[TRACK_SPECIFIC_ITEM] =  g_signal_new("track-specific-item-requested",
                                                G_TYPE_FROM_CLASS (klass),
                                                G_SIGNAL_RUN_LAST,
                                                0,
                                                NULL, NULL,
                                                _sound_service_marshal_VOID__STRING_STRING,
                                                G_TYPE_NONE, 2, G_TYPE_STRING,
                                                G_TYPE_STRING);  
  signals[PLAYER_SPECIFIC_ITEM] =  g_signal_new("player-specific-item-requested",
                                                G_TYPE_FROM_CLASS (klass),
                                                G_SIGNAL_RUN_LAST,
                                                0,
                                                NULL, NULL,
                                                _sound_service_marshal_VOID__STRING_STRING,
                                                G_TYPE_NONE, 2, G_TYPE_STRING,
                                                G_TYPE_STRING);  
}

static void
sound_service_dbus_init (SoundServiceDbus *self)
{
  GError *error = NULL;
  SoundServiceDbusPrivate * priv = SOUND_SERVICE_DBUS_GET_PRIVATE(self);

  priv->connection = NULL;

  /* Fetch the session bus */
  priv->connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);

  if (error != NULL) {
    g_critical ("sound-service-dbus:Unable to connect to the session bus when creating indicator sound service : %s", error->message);
    g_error_free (error);
    return;
  }
  /* register the service on it */
  priv->registration_id = g_dbus_connection_register_object (priv->connection,
                                                             INDICATOR_SOUND_SERVICE_DBUS_OBJECT_PATH,
                                                             interface_info,
                                                             &interface_table,
                                                             self,
                                                             NULL,
                                                             &error);
  if (error != NULL) {
    g_critical ("Unable to register the sound service on DBus: %s", error->message);
    g_error_free (error);
  }
}

DbusmenuMenuitem*
sound_service_dbus_create_root_item (SoundServiceDbus* self, gboolean greeter_mode)
{
  SoundServiceDbusPrivate * priv = SOUND_SERVICE_DBUS_GET_PRIVATE(self);
  priv->greeter_mode = greeter_mode;
  priv->root_menuitem = dbusmenu_menuitem_new();
  DbusmenuServer *server = dbusmenu_server_new (INDICATOR_SOUND_MENU_DBUS_OBJECT_PATH);
  dbusmenu_server_set_root (server, priv->root_menuitem);
  g_object_unref (priv->root_menuitem);
  priv->device = device_new (self);
  return priv->root_menuitem;
}

void
sound_service_dbus_build_sound_menu ( SoundServiceDbus* self,
                                      DbusmenuMenuitem* mute_item,
                                      DbusmenuMenuitem* slider_item,
                                      DbusmenuMenuitem* voip_input_menu_item)
{
  SoundServiceDbusPrivate * priv = SOUND_SERVICE_DBUS_GET_PRIVATE(self);

  // Mute, Volume and Voip widgets
  dbusmenu_menuitem_child_add_position (priv->root_menuitem, mute_item, 0);
  dbusmenu_menuitem_child_add_position (priv->root_menuitem, slider_item, 1);
  dbusmenu_menuitem_child_add_position (priv->root_menuitem, voip_input_menu_item, 2);

  if (!priv->greeter_mode) {
    // Separator
    DbusmenuMenuitem* separator = dbusmenu_menuitem_new();

    dbusmenu_menuitem_property_set (separator,
                                    DBUSMENU_MENUITEM_PROP_TYPE,
                                    DBUSMENU_CLIENT_TYPES_SEPARATOR);
    dbusmenu_menuitem_child_add_position (priv->root_menuitem, separator, 3);
    g_object_unref (separator);

    // Sound preferences dialog
    DbusmenuMenuitem* settings_mi = dbusmenu_menuitem_new();

    dbusmenu_menuitem_property_set( settings_mi,
                                    DBUSMENU_MENUITEM_PROP_LABEL,
                                    _("Sound Settings..."));
    dbusmenu_menuitem_child_append(priv->root_menuitem, settings_mi);
    g_object_unref (settings_mi);  
    g_signal_connect(G_OBJECT(settings_mi), DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,
                     G_CALLBACK(show_sound_settings_dialog), NULL);  
  }
}

/**
show_sound_settings_dialog:
Bring up the gnome volume preferences dialog
**/
static void
show_sound_settings_dialog (DbusmenuMenuitem *mi,
                            gpointer user_data)
{
  GError * error = NULL;
  if (!g_spawn_command_line_async("gnome-volume-control --page=applications", &error) &&
      !g_spawn_command_line_async("gnome-control-center sound", &error) && 
      !g_spawn_command_line_async("xfce4-mixer", &error))
  {
    g_warning("Unable to show dialog: %s", error->message);
    g_error_free(error);
  }
}

static void
sound_service_dbus_dispose (GObject *object)
{
  SoundServiceDbusPrivate *priv = SOUND_SERVICE_DBUS_GET_PRIVATE (object);

  if (priv->connection && priv->registration_id) {
    g_dbus_connection_unregister_object (priv->connection, priv->registration_id);
    priv->registration_id = 0;
  }

  g_clear_object(&priv->connection);

  G_OBJECT_CLASS (sound_service_dbus_parent_class)->dispose (object);
  //TODO dispose of the active sink instance !
  return;
}

static void
sound_service_dbus_finalize (GObject *object)
{
  G_OBJECT_CLASS (sound_service_dbus_parent_class)->finalize (object);
  return;
}


// EMIT STATE SIGNAL
void 
sound_service_dbus_update_sound_state (SoundServiceDbus* self,
                                       SoundState new_state)
{
  SoundServiceDbusPrivate *priv = SOUND_SERVICE_DBUS_GET_PRIVATE (self);

  GVariant* v_output = g_variant_new("(i)", (int)new_state);

  GError * error = NULL;

  if (priv->connection == NULL ||
      g_dbus_connection_is_closed (priv->connection) == TRUE){
    g_critical ("sound_service_dbus_update_sound_state - dbus connection is %s !!",
                priv->connection == NULL? "NULL" : "closed");
    return;
  }

  //g_debug ("emitting state signal with value %i", (int)new_state);
  g_dbus_connection_emit_signal( priv->connection,
                                 NULL,
                                 INDICATOR_SOUND_SERVICE_DBUS_OBJECT_PATH,
                                 INDICATOR_SOUND_DBUS_INTERFACE,
                                 INDICATOR_SOUND_SIGNAL_STATE_UPDATE,
                                 v_output,
                                 &error );
  if (error != NULL) {
    g_critical ("Unable to emit signal because : %s", error->message);
    g_error_free(error);
  }
}

//HANDLE DBUS METHOD CALLS
static void
bus_method_call (GDBusConnection * connection,
                 const gchar * sender,
                 const gchar * path,
                 const gchar * interface,
                 const gchar * method,
                 GVariant * params,
                 GDBusMethodInvocation * invocation,
                 gpointer user_data)
{ 
  SoundServiceDbus* service = SOUND_SERVICE_DBUS(user_data); 
  g_return_if_fail ( IS_SOUND_SERVICE_DBUS(service) );
  GVariant * retval = NULL;
  SoundServiceDbusPrivate *priv = SOUND_SERVICE_DBUS_GET_PRIVATE (service);

  if (g_strcmp0(method, "GetSoundState") == 0) {
    g_debug("Get state -  %i", device_get_state (priv->device));
    retval =  g_variant_new ( "(i)", device_get_state (priv->device));
  }   
  else if (g_strcmp0(method, "BlacklistMediaPlayer") == 0) {    
    gboolean blacklist;
    const gchar* player_name;
    g_variant_get (params, "(&sb)", &player_name, &blacklist);
                   
    g_debug ("BlacklistMediaPlayer - bool %i", blacklist); 
    g_debug ("BlacklistMediaPlayer - name %s", player_name); 
    gboolean result = sound_service_dbus_blacklist_player (service,                                                           
                                                           player_name,
                                                           blacklist);
    retval =  g_variant_new ("(b)", result);
  }
  else if (g_strcmp0(method, "IsBlacklisted") == 0) {
    const gchar* player_name;
    g_variant_get (params, "(&s)", &player_name);

    g_debug ("IsBlacklisted - name %s", player_name);
    gboolean result = sound_service_dbus_is_blacklisted (service,
                                                         player_name);
    retval =  g_variant_new ("(b)", result);
  }
  else if (g_strcmp0(method, "EnableTrackSpecificItems") == 0) {
    g_debug ("EnableTrackSpecificItems");
    gchar* player_object_path;
    gchar* player_id;
    g_variant_get (params, "(os)", &player_object_path, &player_id);
    //g_debug ("object path = %s and id = %s", player_object_path, player_id);
    g_signal_emit (service,
                   signals[TRACK_SPECIFIC_ITEM],
                   0,
                   player_object_path,
                   player_id);
    g_free (player_object_path);
    g_free (player_id);
    
  }
  else if (g_strcmp0(method, "EnablePlayerSpecificItems") == 0) {
    gchar* player_object_path;
    gchar* player_id;
    g_variant_get (params, "(os)", &player_object_path, &player_id);
    g_debug ("PLayer specific item - object path = %s and id = %s",
            player_object_path,
            player_id);
    g_signal_emit (service,
                   signals[PLAYER_SPECIFIC_ITEM],
                   0,
                   player_object_path,
                   player_id);
    g_free (player_object_path);
    g_free (player_id);
  }
  else {
    g_warning("Calling method '%s' on the sound service but it's unknown", method); 
  }
  g_dbus_method_invocation_return_value (invocation, retval);
}

/**
 TODO - Works nicely but refactor into at least two different methods
**/
static gboolean sound_service_dbus_blacklist_player (SoundServiceDbus* self,
                                                     const gchar* player_name,
                                                     gboolean blacklist) 
{
  g_return_val_if_fail (player_name != NULL, FALSE);
  g_return_val_if_fail (IS_SOUND_SERVICE_DBUS (self), FALSE);

  GVariant* the_black_list;
  gboolean result = FALSE;
  GSettings* our_settings;
  GVariantIter iter;
  gchar *str;
  GVariantBuilder builder;

  our_settings  = g_settings_new ("com.canonical.indicator.sound");
  the_black_list = g_settings_get_value (our_settings,
                                         "blacklisted-media-players");  
  g_variant_iter_init (&iter, the_black_list);
  g_variant_builder_init(&builder, G_VARIANT_TYPE_STRING_ARRAY);  

  while (g_variant_iter_loop (&iter, "s", &str)){
    g_variant_builder_add (&builder, "s", str);
  }
  g_variant_iter_init (&iter, the_black_list);

  if (blacklist == TRUE){
    while (g_variant_iter_loop (&iter, "s", &str)){
      g_print ("first pass to check if %s is present\n", str);
      if (g_strcmp0 (player_name, str) == 0){
        // Return if its already there
        g_debug ("we have this already blacklisted, no need to do anything");
        g_variant_builder_clear (&builder);
        g_object_unref (our_settings);
        g_variant_unref (the_black_list);
        return result;
      }
    }
    // Otherwise blacklist it !
    g_debug ("about to blacklist %s", player_name);
    g_variant_builder_add (&builder, "s", player_name);
  }
  else{
    gboolean present = FALSE;
    g_variant_iter_init (&iter, the_black_list);
    g_debug ("attempting to UN-blacklist %s", player_name);
        
    while (g_variant_iter_loop (&iter, "s", &str)){
      if (g_strcmp0 (player_name, str) == 0){      
        present = TRUE;
      }
    }
    // It was not there anyway, return false
    if (present == FALSE){
      g_debug ("it was not blacklisted ?, no need to do anything");
      g_variant_builder_clear (&builder);
      g_object_unref (our_settings);
      g_variant_unref (the_black_list);
      return result;
    }
    
    // Otherwise free the builder and reconstruct ensuring no duplicates.
    g_variant_builder_clear (&builder);  
    g_variant_builder_init (&builder, G_VARIANT_TYPE_STRING_ARRAY);  

    g_variant_iter_init (&iter, the_black_list);
    
    while (g_variant_iter_loop (&iter, "s", &str)){
      if (g_strcmp0 (player_name, str) != 0){            
        g_variant_builder_add (&builder, "s", str);
      }
    }
  }
  GVariant* value = g_variant_builder_end (&builder);
  result = g_settings_set_value (our_settings,
                                 "blacklisted-media-players",
                                 value);

  g_object_unref (our_settings);
  g_variant_unref (the_black_list);
  
  return result;
}

static gboolean sound_service_dbus_is_blacklisted (SoundServiceDbus *self,
                                                   const gchar      *player_name)
{
  GSettings    *our_settings;
  GVariant     *the_black_list;
  GVariantIter  iter;
  gchar        *str;
  gboolean      result = FALSE;

  g_return_val_if_fail (player_name != NULL, FALSE);
  g_return_val_if_fail (IS_SOUND_SERVICE_DBUS (self), FALSE);

  our_settings = g_settings_new ("com.canonical.indicator.sound");
  the_black_list = g_settings_get_value (our_settings,
                                         "blacklisted-media-players");
  g_variant_iter_init (&iter, the_black_list);
  while (g_variant_iter_next (&iter, "s", &str)){
    if (g_strcmp0 (player_name, str) == 0) {
      result = TRUE;
      g_free (str);
      break;
    }
    g_free (str);
  }

  g_object_unref (our_settings);
  g_variant_unref (the_black_list);

  return result;
}

