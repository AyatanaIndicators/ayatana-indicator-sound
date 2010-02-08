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
#include "sound-service-server.h"
#include "common-defs.h"
#include "sound-service-marshal.h"
#include "pulse-manager.h"

typedef struct _SoundServiceDbusPrivate SoundServiceDbusPrivate;

struct _SoundServiceDbusPrivate
{
    DBusGConnection *system_bus;
    DBusGConnection *connection;
    GHashTable *sinks_hash;
};


/* Signals */
enum {
  SINK_INPUT_WHILE_MUTED,  
  SINK_VOLUME_UPDATE,
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
                                                    _sound_service_marshal_VOID__INT_BOOLEAN,
                                                    G_TYPE_NONE, 2, G_TYPE_INT, G_TYPE_BOOLEAN);

    signals[SINK_VOLUME_UPDATE] =  g_signal_new("sink-volume-update",
                                                    G_TYPE_FROM_CLASS (klass),
                                                    G_SIGNAL_RUN_LAST,
                                                    0,
                                                    NULL, NULL,
                                                    g_cclosure_marshal_VOID__DOUBLE,
                                                    G_TYPE_NONE, 1, G_TYPE_DOUBLE);
}

/**
DBUS Method Callbacks
TODO do not see the point in this returning a boolean and also needing a sink index since the service needs to be ultimately aware of what sink is chosen. 
**/
void sound_service_dbus_set_sink_volume(SoundServiceDbus* service, const guint volume_percent, GError** gerror)
{
    g_debug("in the set sink volume method in the sound service dbus!, with volume_percent of %i", volume_percent);
    set_sink_volume(volume_percent);
}

GList *
sound_service_dbus_get_sink_list (SoundServiceDbus *self)
{
  SoundServiceDbusPrivate *priv = SOUND_SERVICE_DBUS_GET_PRIVATE (self);

  return g_hash_table_get_keys (priv->sinks_hash);
}


/**
SIGNALS
Utility methods to emit signals from the service into the ether.
**/
void sound_service_dbus_sink_input_while_muted(SoundServiceDbus* obj, gint sink_index, gboolean value)
{
/*    g_assert((num < LAST_SIGNAL) && (num >= 0));*/
    g_debug("Emitting signal: SINK_INPUT_WHILE_MUTED, with sink_index %i and value %i", sink_index, value);
    g_signal_emit(obj,
                signals[SINK_INPUT_WHILE_MUTED],
                0,
                sink_index,
                value);
}

void sound_service_dbus_update_sink_volume(SoundServiceDbus* obj, gdouble sink_volume)
{
    g_debug("Emitting signal: SINK_VOLUME_UPDATE, with sink_volme %f", sink_volume);
    g_signal_emit(obj,
                signals[SINK_VOLUME_UPDATE],
                0,
                sink_volume);
}

void set_pa_sinks_hash(SoundServiceDbus *self, GHashTable *sinks)
{
    SoundServiceDbusPrivate *priv = SOUND_SERVICE_DBUS_GET_PRIVATE (self);
    priv->sinks_hash = sinks;
}
     
static void
sound_service_dbus_init (SoundServiceDbus *self)
{
    GError *error = NULL;
    SoundServiceDbusPrivate * priv = SOUND_SERVICE_DBUS_GET_PRIVATE(self);

	priv->system_bus = NULL;
	priv->connection = NULL;

    /* Get the system bus */
    priv->system_bus = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
	/* Put the object on DBus */
	priv->connection = dbus_g_bus_get(DBUS_BUS_SESSION, &error);

	if (error != NULL) {
		g_error("Unable to connect to the session bus when creating application indicator: %s", error->message);
		g_error_free(error);
		return;
	}
	dbus_g_connection_register_g_object(priv->connection,
	                                    "/org/ayatana/indicator/sound/service",
	                                    G_OBJECT(self));

    return;
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

