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

[CCode (cheader_filename = "common-defs.h")]
namespace DbusmenuMetadata{
        public const string MENUITEM_TYPE;
        public const string MENUITEM_ARTIST;
        public const string MENUITEM_TITLE;
        public const string MENUITEM_ALBUM;
        public const string MENUITEM_ARTURL;    
        public const string MENUITEM_PLAYER_NAME;
        public const string MENUITEM_PLAYER_ICON;
        public const string MENUITEM_PLAYER_RUNNING;
        public const string MENUITEM_HIDE_TRACK_DETAILS;  
}

[CCode (cheader_filename = "common-defs.h")]
namespace DbusmenuTransport{
        public const string MENUITEM_TYPE;
        public const string MENUITEM_PLAY_STATE;
}

[CCode (cheader_filename = "common-defs.h")]
namespace DbusmenuTrackSpecific{
        public const string MENUITEM_TYPE;
}

[CCode (cheader_filename = "common-defs.h")]
namespace DbusmenuPlaylists{
        public const string MENUITEM_TYPE;
        public const string MENUITEM_TITLE;
        public const string MENUITEM_PLAYLISTS;
}
[CCode (cheader_filename = "common-defs.h")]
namespace DbusmenuPlaylist{
        public const string MENUITEM_PATH;
}

[CCode (cprefix ="Transport", cheader_filename = "common-defs.h")]
namespace Transport{
  public enum Action{
    PREVIOUS,
    PLAY_PAUSE,
    NEXT,
    REWIND,
    FORWIND,
    NO_ACTION
  }
  public enum State{
    PLAYING,
    PAUSED,
    LAUNCHING
  }
}
