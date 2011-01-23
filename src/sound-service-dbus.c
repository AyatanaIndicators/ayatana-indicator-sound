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

#include "gen-sound-service.xml.h"
#include "dbus-shared-names.h"
#include "common-defs.h"
#include "pulse-manager.h"
#include "slider-menu-item.h"
#include "mute-menu-item.h"

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
        SliderMenuItem*     volume_slider_menuitem;
        MuteMenuItem*       mute_menuitem;
        SoundState          current_sound_state;
};

static GDBusNodeInfo *            node_info = NULL;
static GDBusInterfaceInfo *       interface_info = NULL;
static gboolean                   b_startup = TRUE;
#define SOUND_SERVICE_DBUS_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), SOUND_SERVICE_DBUS_TYPE, SoundServiceDbusPrivate))

static void sound_service_dbus_class_init (SoundServiceDbusClass *klass);
static void sound_service_dbus_init       (SoundServiceDbus *self);
static void sound_service_dbus_dispose    (GObject *object);
static void sound_service_dbus_finalize   (GObject *object);

static void sound_service_dbus_build_sound_menu ( SoundServiceDbus* root,
                                                  gboolean mute_update,
                                                  gboolean availability,
                                                  gdouble volume );
static void show_sound_settings_dialog (DbusmenuMenuitem *mi,
                                        gpointer user_data);
static void sound_service_dbus_set_state_from_volume (SoundServiceDbus* self);


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
      g_error("Unable to parse Indicator Service Interface description: %s",
               error->message);
      g_error_free(error);
    }
  }

  if (interface_info == NULL) {
    interface_info = g_dbus_node_info_lookup_interface (node_info,
                                                        INDICATOR_SOUND_DBUS_INTERFACE);

    if (interface_info == NULL) {
      g_error("Unable to find interface '" INDICATOR_SOUND_DBUS_INTERFACE "'");
    }
  }
}

static void
sound_service_dbus_init (SoundServiceDbus *self)
{
  GError *error = NULL;
  SoundServiceDbusPrivate * priv = SOUND_SERVICE_DBUS_GET_PRIVATE(self);

  priv->connection = NULL;

  priv->current_sound_state = UNAVAILABLE;

  /* Fetch the session bus */
  priv->connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);

  if (error != NULL) {
    g_error("sound-service-dbus:Unable to connect to the session bus when creating indicator sound service : %s", error->message);
    g_error_free(error);
    return;
  }
  /* register the service on it */
  g_dbus_connection_register_object (priv->connection,
                                     INDICATOR_SOUND_SERVICE_DBUS_OBJECT_PATH,
                                     interface_info,
                                     &interface_table,
                                     self,
                                     NULL,
                                     &error);
	if (error != NULL) {
		g_error("Unable to register the object to DBus: %s", error->message);
		g_error_free(error);
		return;
	}
}

DbusmenuMenuitem* sound_service_dbus_create_root_item (SoundServiceDbus* self)
{
  SoundServiceDbusPrivate * priv = SOUND_SERVICE_DBUS_GET_PRIVATE(self);
  priv->root_menuitem = dbusmenu_menuitem_new();
  g_debug("Root ID: %d", dbusmenu_menuitem_get_id(priv->root_menuitem));
  DbusmenuServer *server = dbusmenu_server_new(INDICATOR_SOUND_MENU_DBUS_OBJECT_PATH);
  dbusmenu_server_set_root(server, priv->root_menuitem);
  establish_pulse_activities(self);
  return priv->root_menuitem;
}

static void sound_service_dbus_build_sound_menu ( SoundServiceDbus* self,
                                                  gboolean mute_update,
                                                  gboolean availability,
                                                  gdouble volume )
{
  SoundServiceDbusPrivate * priv = SOUND_SERVICE_DBUS_GET_PRIVATE(self);

  // Mute button
  priv->mute_menuitem = mute_menu_item_new ( mute_update, availability);
  dbusmenu_menuitem_child_append (priv->root_menuitem, DBUSMENU_MENUITEM(priv->mute_menuitem));

  // Slider
  priv->volume_slider_menuitem = slider_menu_item_new ( availability, volume );
  dbusmenu_menuitem_child_append (priv->root_menuitem, DBUSMENU_MENUITEM ( priv->volume_slider_menuitem ));

  // Separator
  DbusmenuMenuitem* separator = dbusmenu_menuitem_new();
  dbusmenu_menuitem_property_set( separator,
                                  DBUSMENU_MENUITEM_PROP_TYPE,
                                  DBUSMENU_CLIENT_TYPES_SEPARATOR);
  dbusmenu_menuitem_child_append(priv->root_menuitem, separator);

  // Sound preferences dialog
  DbusmenuMenuitem* settings_mi = dbusmenu_menuitem_new();
  dbusmenu_menuitem_property_set( settings_mi,
                                  DBUSMENU_MENUITEM_PROP_LABEL,
                                  _("Sound Preferences..."));
  dbusmenu_menuitem_child_append(priv->root_menuitem, settings_mi);
  g_signal_connect(G_OBJECT(settings_mi), DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,
                   G_CALLBACK(show_sound_settings_dialog), NULL);
}

/**
show_sound_settings_dialog:
Bring up the gnome volume preferences dialog
**/
static void show_sound_settings_dialog (DbusmenuMenuitem *mi,
                                        gpointer user_data)
{
  GError * error = NULL;
  if (!g_spawn_command_line_async("gnome-volume-control --page=applications", &error) &&
      !g_spawn_command_line_async("xfce4-mixer", &error)) 
  {
    g_warning("Unable to show dialog: %s", error->message);
    g_error_free(error);
  }
}

void sound_service_dbus_update_pa_state ( SoundServiceDbus* self,
                                          gboolean availability,
                                          gboolean mute_update,
                                          gdouble volume )
{
  g_debug("update pa state with availability of %i, mute value of %i and a volume percent is %f", availability, mute_update, volume);
  SoundServiceDbusPrivate * priv = SOUND_SERVICE_DBUS_GET_PRIVATE(self);

  if (b_startup == TRUE) {
    sound_service_dbus_build_sound_menu ( self,
                                          mute_update,
                                          availability,
                                          volume );
    b_startup = FALSE;
    return;
  }

  mute_menu_item_update ( priv->mute_menuitem,
                          mute_update );
  slider_menu_item_update ( priv->volume_slider_menuitem,
                            volume );

  mute_menu_item_enable ( priv->mute_menuitem, availability);
  slider_menu_item_enable ( priv->volume_slider_menuitem, 
                            availability );

  // Emit the signals after the menus are setup/torn down
  // preserve ordering !
  /*sound_service_dbus_update_sink_availability(dbus_interface, sink_available);
  dbus_menu_manager_update_volume(percent);
  sound_service_dbus_update_sink_mute(dbus_interface, sink_muted);
  dbus_menu_manager_update_mute_ui(b_all_muted);*/
}


static void
sound_service_dbus_dispose (GObject *object)
{
  G_OBJECT_CLASS (sound_service_dbus_parent_class)->dispose (object);
  return;
}

static void
sound_service_dbus_finalize (GObject *object)
{
  G_OBJECT_CLASS (sound_service_dbus_parent_class)->finalize (object);
  return;
}

/* A method has been called from our dbus inteface.  Figure out what it
   is and dispatch it. */
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
  //GVariant * retval = NULL;
  //SoundServiceDbusPrivate *priv = SOUND_SERVICE_DBUS_GET_PRIVATE (service);
  // TODO we will need to implement the black_list and state fetch.  
}

// TODO until the pulsemanager has been refactored keep in place the consistent api 
// for it to talk to the UI.
void sound_service_dbus_update_volume(SoundServiceDbus* obj,
                                      gdouble  volume)
{
  SoundServiceDbusPrivate *priv = SOUND_SERVICE_DBUS_GET_PRIVATE (obj);
  slider_menu_item_update (priv->volume_slider_menuitem, volume);
}

void sound_service_dbus_update_sink_mute(SoundServiceDbus* obj,
                                         gboolean mute_update)
{
  SoundServiceDbusPrivate *priv = SOUND_SERVICE_DBUS_GET_PRIVATE (obj);
  mute_menu_item_update (priv->mute_menuitem, mute_update);
}
// TODO: this will be a bit messy until the pa_manager is sorted.
void sound_service_dbus_update_sound_state (SoundServiceDbus* self,
                                            SoundState new_state)
{
  SoundServiceDbusPrivate *priv = SOUND_SERVICE_DBUS_GET_PRIVATE (self);

  if (new_state == AVAILABLE &&
      dbusmenu_menuitem_property_get_bool (priv->mute_menuitem, DBUSMENU_MUTE_MENUITEM_VALUE) == FALSE){
      sound_service_dbus_set_state_from_volume (self);
  }
}

static void sound_service_dbus_set_state_from_volume (SoundServiceDbus* self)
{
  //SoundServiceDbusPrivate *priv = SOUND_SERVICE_DBUS_GET_PRIVATE (self);
}



