/*
 * Copyright 2010 Canonical Ltd.
 *
 * Authors:
 *     Conor Curran <conor.curran@canonical.com>
 *     Ted Gould <ted.gould@canonical.com>
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
        gboolean            mute;
        gboolean            sink_availability;
        DbusmenuMenuitem*   root_menuitem;
        SliderMenuItem*     volume_slider_menuitem;
        MuteMenuItem*       mute_menuitem;
};

static GDBusNodeInfo *            node_info = NULL;
static GDBusInterfaceInfo *       interface_info = NULL;

#define SOUND_SERVICE_DBUS_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), SOUND_SERVICE_DBUS_TYPE, SoundServiceDbusPrivate))

static void sound_service_dbus_class_init (SoundServiceDbusClass *klass);
static void sound_service_dbus_init       (SoundServiceDbus *self);
static void sound_service_dbus_dispose    (GObject *object);
static void sound_service_dbus_finalize   (GObject *object);

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

  priv->mute = FALSE;
  priv->sink_availability = FALSE;

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

DbusmenuMenuitem* sound_service_dbus_construct_menu (SoundServiceDbus* self)
{
  SoundServiceDbusPrivate * priv = SOUND_SERVICE_DBUS_GET_PRIVATE(self);
  priv->root_menuitem = dbusmenu_menuitem_new();
  g_debug("Root ID: %d", dbusmenu_menuitem_get_id(priv->root_menuitem));
  DbusmenuServer *server = dbusmenu_server_new(INDICATOR_SOUND_MENU_DBUS_OBJECT_PATH);
  dbusmenu_server_set_root(server, priv->root_menuitem);
  establish_pulse_activities(self);
  return priv->root_menuitem;
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
  GVariant * retval = NULL;
  SoundServiceDbusPrivate *priv = SOUND_SERVICE_DBUS_GET_PRIVATE (service);
  
  if (g_strcmp0(method, "GetSinkMute") == 0) {
    g_debug("Get sink mute - sound service dbus!,about to send over mute_value of  %i", priv->mute);
    retval =  g_variant_new ( "(b)", priv->mute);    
  } 
  else if (g_strcmp0(method, "GetSinkAvailability") == 0) {
    g_debug("Get sink availability - sound service dbus!, about to send over availability_value of  %i", priv->sink_availability);
    retval =  g_variant_new ( "(b)", priv->sink_availability);
  }
  else {
    g_warning("Calling method '%s' on the sound service but it's unknown", method); 
  }
  g_dbus_method_invocation_return_value(invocation, retval);
}


/**
SIGNALS
Utility methods to emit signals from the service into the ether.
**/
void sound_service_dbus_sink_input_while_muted(SoundServiceDbus* obj,
                                               gboolean block_value)
{
  g_debug("Emitting signal: SINK_INPUT_WHILE_MUTED, with  block_value: %i",
           block_value);
  SoundServiceDbusPrivate *priv = SOUND_SERVICE_DBUS_GET_PRIVATE (obj);
  GVariant* v_output = g_variant_new("(b)", block_value);

  GError * error = NULL;

  g_dbus_connection_emit_signal( priv->connection,
                                 NULL,
                                 INDICATOR_SOUND_MENU_DBUS_OBJECT_PATH,
                                 INDICATOR_SOUND_DBUS_INTERFACE,
                                 INDICATOR_SOUND_SIGNAL_SINK_INPUT_WHILE_MUTED,
                                  v_output,
                                  &error );
  if (error != NULL) {
    g_error("Unable to emit signal 'sinkinputwhilemuted' because : %s", error->message);
    g_error_free(error);
    return;
  }
}

void sound_service_dbus_update_sink_mute(SoundServiceDbus* obj,
                                         gboolean sink_mute)
{
  g_debug("Emitting signal: SINK_MUTE_UPDATE, with sink mute %i", sink_mute);
  SoundServiceDbusPrivate *priv = SOUND_SERVICE_DBUS_GET_PRIVATE (obj);
  priv->mute = sink_mute;

  GVariant* v_output = g_variant_new("(b)", sink_mute);
  GError * error = NULL;
  g_dbus_connection_emit_signal( priv->connection,
                                 NULL,
                                 INDICATOR_SOUND_SERVICE_DBUS_OBJECT_PATH,
                                 INDICATOR_SOUND_DBUS_INTERFACE,
                                 INDICATOR_SOUND_SIGNAL_SINK_MUTE_UPDATE,
                                  v_output,
                                  &error );
  if (error != NULL) {
    g_error("Unable to emit signal 'sinkmuteupdate' because : %s", error->message);
    g_error_free(error);
    return;
  }
}

void sound_service_dbus_update_sink_availability(SoundServiceDbus* obj,
                                                 gboolean sink_availability)
{
  g_debug("Emitting signal: SinkAvailableUpdate, with  %i", sink_availability);
  SoundServiceDbusPrivate *priv = SOUND_SERVICE_DBUS_GET_PRIVATE (obj);
  priv->sink_availability = sink_availability;
  
  GVariant* v_output = g_variant_new("(b)", priv->sink_availability);
  GError * error = NULL;
  
  g_dbus_connection_emit_signal( priv->connection,
                                 NULL,
                                 INDICATOR_SOUND_SERVICE_DBUS_OBJECT_PATH,
                                 INDICATOR_SOUND_DBUS_INTERFACE,
                                 INDICATOR_SOUND_SIGNAL_SINK_AVAILABLE_UPDATE,
                                  v_output,
                                  &error );
  if (error != NULL) {
    g_error("Unable to emit signal 'SinkAvailableUpdate' because : %s", error->message);
    g_error_free(error);
    return;
  }
}



