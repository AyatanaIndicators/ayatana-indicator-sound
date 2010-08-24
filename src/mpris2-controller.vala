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
using DBus;


[DBus (name = "org.mpris.MediaPlayer2")]
public interface MprisRoot : DBus.Object {
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
public interface MprisPlayer : DBus.Object {

	// properties
	public abstract HashTable<string, Value?> Metadata{owned get; set;}
	public abstract int32 Position{owned get; set;}
	public abstract string PlaybackStatus{owned get; set;}	
	// methods
	public abstract void SetPosition(DBus.ObjectPath path, int64 pos) throws DBus.Error;
	public abstract void PlayPause() throws DBus.Error;
	public abstract void Pause() throws DBus.Error;
	public abstract void Next() throws DBus.Error;
	public abstract void Previous() throws DBus.Error;
	// signals
	public signal void Seeked(int64 new_position);
}

[DBus (name = "org.freedesktop.DBus.Properties")]
public interface FreeDesktopProperties : DBus.Object{
	// signals
	public signal void PropertiesChanged(string source, HashTable<string, Value?> changed_properties, string[] invalid);
}

/*
 This class will entirely replace mpris-controller.vala hence why there is no
 point in trying to get encorporate both into the same object model. 
 */
public class Mpris2Controller : GLib.Object
{		
	public static const string root_interface = "org.mpris.MediaPlayer2" ;	
	public MprisRoot mpris2_root {get; construct;}		
	public MprisPlayer player {get; construct;}		
	public PlayerController owner {get; construct;}
	public FreeDesktopProperties properties_interface {get; construct;}
	
	public Mpris2Controller(PlayerController ctrl)
	{
		GLib.Object(owner: ctrl);
	}
	
	construct{
    try {
      var connection = DBus.Bus.get (DBus.BusType.SESSION);
			this.mpris2_root = (MprisRoot) connection.get_object (root_interface.concat(".").concat(this.owner.name.down()),
				                                             "/org/mpris/MediaPlayer2",
				                                             root_interface);						
			this.player = (MprisPlayer) connection.get_object (root_interface.concat(".").concat(this.owner.name.down()),
				                                               "/org/mpris/MediaPlayer2",
				                                               root_interface.concat(".Player"));						
			this.player.Seeked += onSeeked;

			this.properties_interface = (FreeDesktopProperties) connection.get_object(root_interface.concat(".").concat(this.owner.name.down()),
			                                                                          "/org/mpris/MediaPlayer2",
			                                                                          "org.freedesktop.DBus.Properties");
			this.properties_interface.PropertiesChanged += property_changed_cb;			
			
		} catch (DBus.Error e) {
      	error("Problems connecting to the session bus - %s", e.message);
    }		
	}

	public void property_changed_cb(string interface_source, HashTable<string, Value?> changed_properties, string[] invalid )
	{	
		debug("properties-changed for interface %s", interface_source);
		if(changed_properties == null || interface_source.has_prefix(this.root_interface) == false){
			warning("Property-changed hash is null or this is an interface that concerns us");
			return;
		}
		Value? play_v = changed_properties.lookup("PlaybackStatus");
		if(play_v != null){
			string state = play_v.get_string();		
			debug("new playback state = %s", state);			
			int p = this.determine_play_state(state);
			(this.owner.custom_items[PlayerController.widget_order.TRANSPORT] as TransportMenuitem).change_play_state(p);			
			(this.owner.custom_items[PlayerController.widget_order.SCRUB] as ScrubMenuitem).update_playstate(p);			
		}
		
		Value? pos_v = changed_properties.lookup("Position");
		if(pos_v != null){
			int64 pos = pos_v.get_int64();
			debug("new position = %i", (int)pos);
			(this.owner.custom_items[PlayerController.widget_order.SCRUB] as ScrubMenuitem).update_position((int32)pos);						
		}

		Value? meta_v = changed_properties.lookup("Metadata");
		if(meta_v != null){
			GLib.HashTable<string, Value?> changed_updates = clean_metadata();	

			MetadataMenuitem meta = this.owner.custom_items[PlayerController.widget_order.METADATA] as MetadataMenuitem;
			meta.reset(MetadataMenuitem.attributes_format());					
			
			this.owner.custom_items[PlayerController.widget_order.METADATA].update(changed_updates,
			                          																						 MetadataMenuitem.attributes_format());			
			this.owner.custom_items[PlayerController.widget_order.SCRUB].reset(ScrubMenuitem.attributes_format());	
			this.owner.custom_items[PlayerController.widget_order.SCRUB].update(changed_updates,
				                    																							ScrubMenuitem.attributes_format());			
		
			(this.owner.custom_items[PlayerController.widget_order.SCRUB] as ScrubMenuitem).update_playstate(this.determine_play_state(this.player.PlaybackStatus));			
			
		}
	}

	private GLib.HashTable<string, Value?> clean_metadata()
	{ 
		GLib.HashTable<string, Value?> changed_updates = this.player.Metadata; 
		string[] artists = (string[])this.player.Metadata.lookup("xesam:artist");
		string display_artists = string.joinv(", ", artists);
		changed_updates.replace("xesam:artist", display_artists);
		debug("artist : %s", display_artists);
		int64 duration = this.player.Metadata.lookup("mpris:length").get_int64();
		changed_updates.replace("mpris:length", duration/1000000); 
		return changed_updates;
	}
	
	
	private int determine_play_state(string status){
		if(status == null)
			return 1;
		
		if(status != null && status == "Playing"){
			debug("determine play state - state = %s", status);
			return 0;
		}
		return 1;		
	}
	
	public void initial_update()
	{
		int32 status;
		if(this.player.PlaybackStatus == null){
			status = 1;
		}
		else{
			status = determine_play_state(this.player.PlaybackStatus);
		}
		debug("initial update - play state %i", status);
		
		(this.owner.custom_items[PlayerController.widget_order.TRANSPORT] as TransportMenuitem).change_play_state(status);
		var cleaned_metadata = this.clean_metadata();
		this.owner.custom_items[PlayerController.widget_order.METADATA].update(cleaned_metadata,
			                          MetadataMenuitem.attributes_format());
		this.owner.custom_items[PlayerController.widget_order.SCRUB].update(cleaned_metadata,
			                      ScrubMenuitem.attributes_format());		
	}

	public void transport_event(TransportMenuitem.action command)
	{		
		debug("transport_event input = %i", (int)command);
		if(command == TransportMenuitem.action.PLAY_PAUSE){
			debug("transport_event PLAY_PAUSE");
			try{
				this.player.PlayPause();							
			}
			catch(DBus.Error error){
				warning("DBus Error calling the player objects PlayPause method %s",
				        error.message);
			}			
		}
		else if(command == TransportMenuitem.action.PREVIOUS){
			try{
				this.player.Previous();
			}
			catch(DBus.Error error){
				warning("DBus Error calling the player objects Previous method %s",
				        error.message);
			}							
		}
		else if(command == TransportMenuitem.action.NEXT){
			try{
				this.player.Next();
			}
			catch(DBus.Error error){
				warning("DBus Error calling the player objects Next method %s",
				      	error.message);
			}								
		}	
	}
	/**
		TODO: SetPosition on the player object is not working with rhythmbox,
	  runtime error - "dbus function not supported"
	 */
	public void set_position(double position)
	{			
		debug("Set position with pos (0-100) %f", position);
		Value? time_value = this.player.Metadata.lookup("mpris:length");
		if(time_value == null){
			warning("Can't fetch the duration of the track therefore cant set the position");
			return;
		}
		// work in microseconds (scale up by 10 TTP-of 6)
		int64 total_time = time_value.get_int64();
		debug("total time of track = %i", (int)total_time);				
		double new_time_position = total_time * (position/100.0);
		debug("new position = %f", (new_time_position));		

		Value? v = this.player.Metadata.lookup("mpris:trackid");
		if(v != null){
			if(v.holds (typeof (string))){
				DBus.ObjectPath path = new ObjectPath(v.get_string());
				try{
					this.player.SetPosition(path, (int64)(new_time_position));
					ScrubMenuitem scrub = this.owner.custom_items[PlayerController.widget_order.SCRUB] as ScrubMenuitem;
					scrub.update_position(((int32)new_time_position) / 1000);			
				}
				catch(DBus.Error e){
					error("DBus Error calling the player objects SetPosition method %s",
						     e.message);
				}							
			}
		}			        
	}

	public void onSeeked(int64 position){
		debug("Seeked signal callback with pos = %i", (int)position/1000);
		ScrubMenuitem scrub = this.owner.custom_items[PlayerController.widget_order.SCRUB] as ScrubMenuitem;
		scrub.update_position((int32)position/1000);			
	}
	
	public bool connected()
	{
		return (this.player != null && this.mpris2_root != null);
	}

		
	public bool was_successfull(){
		if(this.mpris2_root == null ||this.player == null){
			return false;
		}
		return true;
	}

	public void expose()
	{
		if(this.connected() == true){
			try{
				this.mpris2_root.Raise();
			}
			catch(DBus.Error e){
				error("Exception thrown while calling root function Raise - %s", e.message);
			}
		}
	}
}



