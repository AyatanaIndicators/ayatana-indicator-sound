/*
 * Copyright 2010 Canonical Ltd.
 *
 * Authors:
 *     Conor Curran <conor.curran@canonical.com>
 *     Cody Russell <crussell@canonical.com>
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

#include <dbus/dbus-glib.h>
#include "dbus-shared-names.h"
#include "sound-service-dbus.h"
#include "common-defs.h"
#include "pulse-manager.h"

// DBUS methods
static gboolean sound_service_dbus_get_sink_mute(SoundServiceDbus* service, gboolean* mute_input, GError** gerror);
static gboolean sound_service_dbus_get_sink_availability(SoundServiceDbus* service, gboolean* availability_input, GError** gerror);

#include "sound-service-server.h"

typedef struct _SoundServiceDbusPrivate SoundServiceDbusPrivate;

struct _SoundServiceDbusPrivate {
  DBusGConnection *connection;
  gboolean        mute;
  gboolean        sink_availability;
};


/* Signals */
enum {
  SINK_INPUT_WHILE_MUTED,
  SINK_MUTE_UPDATE,
  SINK_AVAILABLE_UPDATE,
  LAST_SIGNAL
};


static guint signals[LAST_SIGNAL] = { 0 };

#define SOUND_SERVICE_DBUS_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), SOUND_SERVICE_DBUS_TYPE, SoundServiceDbusPrivate))

static void sound_service_dbus_class_init (SoundServiceDbusClass *klass);
static void sound_service_dbus_init       (SoundServiceDbus *self);
static void sound_service_dbus_dispose    (GObject *object);
static void sound_service_dbus_finalize   (GObject *object);

/* GObject Boilerplate */
G_DEFINE_TYPE (SoundServiceDbus, sound_service_dbus, G_TYPE_OBJECT);

static void
sound_service_dbus_class_init (SoundServiceDbusClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (object_class, sizeof(SoundServiceDbusPrivate));

  object_class->dispose = sound_service_dbus_dispose;
  object_class->finalize = sound_service_dbus_finalize;

  g_assert(klass != NULL);
  dbus_g_object_type_install_info(SOUND_SERVICE_DBUS_TYPE,
                                  &dbus_glib__sound_service_server_object_info);

  signals[SINK_INPUT_WHILE_MUTED] =  g_signal_new("sink-input-while-muted",
                                     G_TYPE_FROM_CLASS (klass),
                                     G_SIGNAL_RUN_LAST,
                                     0,
                                     NULL, NULL,
                                     g_cclosure_marshal_VOID__BOOLEAN,
                                     G_TYPE_NONE, 1, G_TYPE_BOOLEAN);

  signals[SINK_MUTE_UPDATE] =  g_signal_new("sink-mute-update",
                               G_TYPE_FROM_CLASS (klass),
                               G_SIGNAL_RUN_LAST,
                               0,
                               NULL, NULL,
                               g_cclosure_marshal_VOID__BOOLEAN,
                               G_TYPE_NONE, 1, G_TYPE_BOOLEAN);
  signals[SINK_AVAILABLE_UPDATE] =  g_signal_new("sink-available-update",
                                    G_TYPE_FROM_CLASS (klass),
                                    G_SIGNAL_RUN_LAST,
                                    0,
                                    NULL, NULL,
                                    g_cclosure_marshal_VOID__BOOLEAN,
                                    G_TYPE_NONE, 1, G_TYPE_BOOLEAN);
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
  priv->connection = dbus_g_bus_get(DBUS_BUS_SESSION, &error);

  if (error != NULL) {
    g_error("sound-service-dbus:Unable to connect to the session bus when creating indicator sound service : %s", error->message);
    g_error_free(error);
    return;
  }
  /* register the service on it */
  dbus_g_connection_register_g_object(priv->connection,
                                      INDICATOR_SOUND_SERVICE_DBUS_OBJECT,
                                      G_OBJECT(self));
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


static gboolean sound_service_dbus_get_sink_mute (SoundServiceDbus *self, gboolean *mute_input, GError** gerror)
{
  SoundServiceDbusPrivate *priv = SOUND_SERVICE_DBUS_GET_PRIVATE (self);
  g_debug("Get sink mute - sound service dbus!, about to send over mute_value of  %i", priv->mute);
  *mute_input = priv->mute;
  return TRUE;
}

static gboolean sound_service_dbus_get_sink_availability (SoundServiceDbus *self, gboolean *availability_input, GError** gerror)
{
  SoundServiceDbusPrivate *priv = SOUND_SERVICE_DBUS_GET_PRIVATE (self);
  g_debug("Get sink availability - sound service dbus!, about to send over availability_value of  %i", priv->sink_availability);
  *availability_input = priv->sink_availability;
  return TRUE;
}

/**
SIGNALS
Utility methods to emit signals from the service into the ether.
**/
void sound_service_dbus_sink_input_while_muted(SoundServiceDbus* obj,  gboolean block_value)
{
  /*    g_debug("Emitting signal: SINK_INPUT_WHILE_MUTED, with  block_value: %i", block_value);*/
  g_signal_emit(obj,
                signals[SINK_INPUT_WHILE_MUTED],
                0,
                block_value);
}

void sound_service_dbus_update_sink_mute(SoundServiceDbus* obj, gboolean sink_mute)
{
  /*    g_debug("Emitting signal: SINK_MUTE_UPDATE, with sink mute %i", sink_mute);*/

  SoundServiceDbusPrivate *priv = SOUND_SERVICE_DBUS_GET_PRIVATE (obj);
  priv->mute = sink_mute;

  g_signal_emit(obj,
                signals[SINK_MUTE_UPDATE],
                0,
                priv->mute);
}

void sound_service_dbus_update_sink_availability(SoundServiceDbus* obj, gboolean sink_availability)
{
  /*    g_debug("Emitting signal: SINK_AVAILABILITY_UPDATE, with value %i", sink_availability);*/

  SoundServiceDbusPrivate *priv = SOUND_SERVICE_DBUS_GET_PRIVATE (obj);
  priv->sink_availability = sink_availability;

  g_signal_emit(obj,
                signals[SINK_AVAILABLE_UPDATE],
                0,
                priv->sink_availability);
}



