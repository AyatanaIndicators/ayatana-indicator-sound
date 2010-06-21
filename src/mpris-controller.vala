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
  public dynamic DBus.Object mpris_player;
	private PlayerController controller;
  struct status {
    public int32 playback;
    public int32 shuffle;
    public int32 repeat;
    public int32 endless;
  }
		
	public MprisController(string name, PlayerController controller, string mpris_interface="org.freedesktop.MediaPlayer"){
    try {
      this.connection = DBus.Bus.get (DBus.BusType.SESSION);
    } catch (Error e) {
      error("Problems connecting to the session bus - %s", e.message);
    }		
		this.controller = controller;
		this.mpris_player = this.connection.get_object ("org.mpris.".concat(name.down()) , "/Player", mpris_interface);				
    this.mpris_player.TrackChange += onTrackChange;	
    this.mpris_player.StatusChange += onStatusChange;		
	}


	private void onTrackChange(dynamic DBus.Object mpris_client, HashTable<string,Value?> ht)
	{
		this.controller.custom_items[this.controller.METADATA].update(ht,
		                            MetadataMenuitem.attributes_format());
	}

	/**
	 * TRUE  => Playing
	 * FALSE => Paused
	 **/
	public void toggle_playback(bool state)
	{
		if(state == true){
			debug("about to play");
			this.mpris_player.Play();				
		}
		else{
			debug("about to pause");
			this.mpris_player.Pause();						
		}		
	}
	
	private void onStatusChange(dynamic DBus.Object mpris_client, status st)
  {
    debug("onStatusChange - signal received");	
		status* status = &st;
		unowned ValueArray ar = (ValueArray)status;
		
		bool play_state = (ar.get_nth(0).get_int() == 1);
		debug("onStatusChange - play state %s", play_state.to_string());
		HashTable<string, Value?> ht = new HashTable<string, Value?>(str_hash, str_equal);
		Value v = Value(typeof(bool));
		v.set_boolean(play_state);
		ht.insert("state", play_state); 
		this.controller.custom_items[this.controller.TRANSPORT].update(ht,
		                             TransportMenuitem.attributes_format());
	}
	
}
