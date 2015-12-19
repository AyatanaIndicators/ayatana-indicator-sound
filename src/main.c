/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <glib.h>
#include <locale.h>
#include <libnotify/notify.h>

#include "indicator-sound-service.h"
#include "config.h"

static IndicatorSoundService * service = NULL;

static gboolean
sigterm_handler (gpointer data)
{
	g_debug("Got SIGTERM");
	g_main_loop_quit((GMainLoop *)data);
	return G_SOURCE_REMOVE;
}

static void
on_name_lost(GDBusConnection * connection,
             const gchar * name,
             gpointer user_data)
{
	g_warning("Name lost or unable to acquire bus: %s", name);
	g_main_loop_quit((GMainLoop *)user_data);
}

static void
on_bus_acquired(GDBusConnection *connection,
                const gchar *name,
                gpointer user_data)
{
	MediaPlayerList * playerlist = NULL;
	IndicatorSoundOptions * options = NULL;
	VolumeControlPulse * volume = NULL;
	AccountsServiceUser * accounts = NULL;
   

	if (g_strcmp0("lightdm", g_get_user_name()) == 0) {
		playerlist = MEDIA_PLAYER_LIST(media_player_list_greeter_new());
	} else {
		playerlist = MEDIA_PLAYER_LIST(media_player_list_mpris_new());
		accounts = accounts_service_user_new();
	}

	options = indicator_sound_options_gsettings_new();
	volume = volume_control_pulse_new(options);

	service = indicator_sound_service_new (playerlist, volume, accounts, options);

	g_clear_object(&playerlist);
	g_clear_object(&options);
	g_clear_object(&volume);
	g_clear_object(&accounts);
}

int
main (int argc, char ** argv)
{
	GMainLoop * loop = NULL;

	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	setlocale (LC_ALL, "");
	bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);

	/* Build Mainloop */
	loop = g_main_loop_new(NULL, FALSE);

	g_unix_signal_add(SIGTERM, sigterm_handler, loop); 

	/* Initialize libnotify */
	notify_init ("indicator-sound");

	g_bus_own_name(G_BUS_TYPE_SESSION,
		"com.canonical.indicator.sound",
		G_BUS_NAME_OWNER_FLAGS_NONE,
		on_bus_acquired,
		NULL, /* name acquired */
		on_name_lost,
		loop,
		NULL);

	g_main_loop_run(loop);

	g_clear_object(&service);

	notify_uninit();

	return 0;
}

