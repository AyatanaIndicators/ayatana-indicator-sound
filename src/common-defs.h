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


/* constants used for signals on the dbus. This file is shared between client and server implementation */
#define SIGNAL_SINK_INPUT_WHILE_MUTED "SinkInputWhileMuted"
#define SIGNAL_SINK_VOLUME_UPDATE "SinkVolumeUpdate"
#define SIGNAL_SINK_MUTE_UPDATE "SinkMuteUpdate"
#define SIGNAL_SINK_AVAILABLE_UPDATE "SinkAvailableUpdate"

/* DBUS Custom Items */
#define DBUSMENU_SLIDER_MENUITEM_TYPE          	"x-canonical-ido-slider-item"
#define DBUSMENU_TRANSPORT_MENUITEM_TYPE       	"x-canonical-transport-bar"
#define DBUSMENU_TRANSPORT_MENUITEM_PLAY_STATE  "x-canonical-transport-play-state"

#define DBUSMENU_METADATA_MENUITEM_TYPE  				"x-canonical-metadata-menu-item"
#define DBUSMENU_METADATA_MENUITEM_TEXT_ARTIST  "x-canonical-metadata-text-artist"
#define DBUSMENU_METADATA_MENUITEM_TEXT_TITLE  	"x-canonical-metadata-text-title"
#define DBUSMENU_METADATA_MENUITEM_TEXT_ALBUM   "x-canonical-metadata-text-album"
#define DBUSMENU_METADATA_MENUITEM_ARTURL  			"x-canonical-metadata-arturl"

#define DBUSMENU_TITLE_MENUITEM_TYPE          	"x-canonical-sound-menu-player-title-menu-item"
#define DBUSMENU_TITLE_MENUITEM_TEXT_NAME       "x-canonical-sound-menu-player-title-name"
