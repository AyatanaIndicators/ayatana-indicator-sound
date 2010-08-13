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

[DBus (name = "org.mpris.MediaPlayer2")]
public interface MprisRoot : Object {
	// properties
	public abstract bool HasTracklist{owned get; set;}
	public abstract bool CanQuit{owned get; set;}
	public abstract bool CanRaise{owned get; set;}
	public abstract string Identity{owned get; set;}
	public abstract string DesktopEntry{owned get; set;}	
	// methods
	public abstract void Quit() throws DBus.Error;
	public abstract void Raise() throws DBus.Error;
}

[DBus (name = "org.mpris.MediaPlayer2.Player")]
public interface MprisPlayer : Object {
	
	public abstract HashTable<string, Value?> Metadata{owned get; set;}
	public abstract int32 Position{owned get; set;}
	public abstract string PlaybackStatus{owned get; set;}	
	
	public abstract void SetPosition(string prop, int32 pos) throws DBus.Error;
	public abstract void PlayPause() throws DBus.Error;
	public abstract void Pause() throws DBus.Error;
	public abstract void Next() throws DBus.Error;
	public abstract void Previous() throws DBus.Error;

	public signal void Seeked(int new_position);
}

/*
 This class will entirely replace mpris-controller.vala hence why there is no
 point in trying to get encorporate both into the same object model. 
 */
public class Mpris2Controller : GLib.Object
{		
	public static const string root_interface = "org.mpris.MediaPlayer2" ;	
	public MprisRoot mpris2_root {get; construct;}		
	public MprisPlayer mpris2_player {get; construct;}		
	public PlayerController owner {get; construct;}	
	
	public Mpris2Controller(PlayerController ctrl)
	{
		Object(owner: ctrl);
	}
	
	construct{
    try {
      var connection = DBus.Bus.get (DBus.BusType.SESSION);
			this.mpris2_root = (MprisRoot) connection.get_object (root_interface.concat(".").concat(this.owner.name.down()),
				                                             "/org/mpris/MediaPlayer2",
				                                             root_interface);						
			this.mpris2_player = (MprisPlayer) connection.get_object (root_interface.concat(".").concat(this.owner.name.down()),
				                                               "/org/mpris/MediaPlayer2/Player",
				                                               root_interface.concat(".Player"));					                                               
			this.mpris2_player.Seeked += onSeeked;
			this.mpris2_player.notify["PlaybackStatus"].connect (property_changed);
			
		} catch (DBus.Error e) {
      error("Problems connecting to the session bus - %s", e.message);
    }		
	}

	public void onSeeked(int position){
		debug("Seeked signal callback");
	}

	public void property_changed(Object mpris_player, ParamSpec new_status){
		debug("playback status changed, %s", new_status.get_name());		
	}
	
	public bool was_successfull(){
		if(this.mpris2_root == null ||this.mpris2_player == null){
			return false;
		}
		return true;
	}

	private int determine_play_state(){
		string status = this.mpris2_player.PlaybackStatus;
		if(status == "Playing"){
			return 0;
		}
		return 1;		
	}
	
	public void initial_update()
	{
		int32 p = determine_play_state();
		debug("initial update - play state %i", p);
		
		(this.owner.custom_items[PlayerController.widget_order.TRANSPORT] as TransportMenuitem).change_play_state(p);
		this.owner.custom_items[PlayerController.widget_order.METADATA].update(this.mpris2_player.Metadata,
			                          MetadataMenuitem.attributes_format());
		this.owner.custom_items[PlayerController.widget_order.SCRUB].update(this.mpris2_player.Metadata,
			                      ScrubMenuitem.attributes_format());		
		ScrubMenuitem scrub = this.owner.custom_items[PlayerController.widget_order.SCRUB] as ScrubMenuitem;
		scrub.update_position(this.mpris2_player.Position);
	
	}

	public void transport_event(TransportMenuitem.action command)
	{		
		debug("transport_event input = %i", (int)command);
		if(command == TransportMenuitem.action.PLAY_PAUSE){
			debug("transport_event PLAY_PAUSE");
			try{
				this.mpris2_player.PlayPause();							
			}
			catch(DBus.Error error){
				warning("DBus Error calling the player objects PlayPause method %s",
				        error.message);
			}			
		}
		else if(command == TransportMenuitem.action.PREVIOUS){
			try{
				this.mpris2_player.Previous();
			}
			catch(DBus.Error error){
				warning("DBus Error calling the player objects Previous method %s",
				        error.message);
			}							
		}
		else if(command == TransportMenuitem.action.NEXT){
			try{
				this.mpris2_player.Next();
			}
			catch(DBus.Error error){
				warning("DBus Error calling the player objects Next method %s",
				      	error.message);
			}								
		}	
	}

	public void set_position(double position)
	{			
		debug("Set position with pos (0-100) %f", position);
		HashTable<string, Value?> data = this.mpris2_player.Metadata;
		Value? time_value = data.lookup("time");
		if(time_value == null){
			warning("Can't fetch the duration of the track therefore cant set the position");
			return;
		}
		uint32 total_time = time_value.get_uint();
		debug("total time of track = %i", (int)total_time);				
		double new_time_position = total_time * position/100.0;
		debug("new position = %f", (new_time_position * 1000));		

		Value? v = this.mpris2_player.Metadata.lookup("trackid");
		if(v != null){
			if(v.holds (typeof (int))){
				debug("the trackid = %i", v.get_int());			
			}
			else if(v.holds (typeof (string))){
				debug("the trackid = %s", v.get_string());
			}
		}			        
		/*this.mpris2_player.SetPosition((int32)(new_time_position));
		ScrubMenuitem scrub = this.owner.custom_items[PlayerController.widget_order.SCRUB] as ScrubMenuitem;
		scrub.update_position(this.mpris2_player.Position);				
		*/
	}
	
	public bool connected()
	{
		return (this.mpris2_player != null);
	}
		
}



