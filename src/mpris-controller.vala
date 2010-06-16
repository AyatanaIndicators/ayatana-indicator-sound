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
using Gee;

public class MprisController : GLib.Object
{
  private DBus.Connection connection;
  private dynamic DBus.Object mpris_player;
	private PlayerController controller;
	
	public MprisController(string name, PlayerController controller, string mpris_interface="org.freedesktop.MediaPlayer"){
    try {
      this.connection = DBus.Bus.get (DBus.BusType.SESSION);
    } catch (Error e) {
      error("Problems connecting to the session bus - %s", e.message);
    }		
		this.controller = controller;
		this.mpris_player = this.connection.get_object ("org.mpris.".concat(name.down()) , "/Player", mpris_interface);				
    this.mpris_player.TrackChange += onTrackChange;	
		this.controller.update_playing_info(get_track_data());
	}

	public HashMap<string, string> get_track_data()
	{
		return format_metadata(this.mpris_player.GetMetadata());
	}

	private void onTrackChange(dynamic DBus.Object mpris_client, HashTable<string,Value?> ht)
	{
		this.controller.update_playing_info(format_metadata(ht));
	}

	private static HashMap<string, string> format_metadata(HashTable<string,Value?> data)
	{
		HashMap<string,string> results = new HashMap<string, string>();
		debug("format_metadata - title = %s", (string)data.lookup("title"));
		results.set("title", (string)data.lookup("title"));
    results.set("artist", (string)data.lookup("artist"));
    results.set("album", (string)data.lookup("album"));
    results.set("arturl", (string)data.lookup("arturl"));
		return results;                          				
	}
	
}
