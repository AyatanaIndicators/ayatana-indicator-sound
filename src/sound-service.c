/*
This service primarily controls PulseAudio and is driven by the sound indicator menu on the panel.
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

#include "sound-service.h"
#include "dbus-menu-manager.h"
#include "pulse-manager.h"
#include "music-player-bridge.h"

static GMainLoop *mainloop = NULL;

/**********************************************************************************************************************/
//    Init and exit functions
/**********************************************************************************************************************/

/**
service_shutdown:
When the service interface starts to shutdown, we
should follow it.
**/

void
service_shutdown (IndicatorService *service, gpointer user_data)
{
  if (mainloop != NULL) {
    g_debug("Service shutdown !");
    close_pulse_activites();
    g_main_loop_quit(mainloop);
  }
  return;
}

/**
main:
**/
int
main (int argc, char ** argv)
{
  g_type_init();

  setlocale (LC_ALL, "");
  bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
  textdomain (GETTEXT_PACKAGE);

  IndicatorService *service = indicator_service_new_version(INDICATOR_SOUND_DBUS_NAME,
                              INDICATOR_SOUND_DBUS_VERSION);
  g_signal_connect(G_OBJECT(service),
                   INDICATOR_SERVICE_SIGNAL_SHUTDOWN,
                   G_CALLBACK(service_shutdown), NULL);

  DbusmenuMenuitem* root_menuitem = dbus_menu_manager_setup();
  MusicPlayerBridge* server = music_player_bridge_new();
  music_player_bridge_set_root_menu_item(server, root_menuitem);

  // Run the loop
  mainloop = g_main_loop_new(NULL, FALSE);
  g_main_loop_run(mainloop);

  return 0;
}




