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
	public signal void PropertiesChanged(string source, HashTable<string,
                                       Value?> changed_properties,
                                       string[] invalid);
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
			this.properties_interface = (FreeDesktopProperties) connection.get_object("org.freedesktop.Properties.PropertiesChanged",//root_interface.concat(".").concat(this.owner.name.down()),
			                                                                          "/org/mpris/MediaPlayer2");                                                                                
			this.properties_interface.PropertiesChanged += property_changed_cb;			
			
		} catch (DBus.Error e) {
      	error("Problems connecting to the session bus - %s", e.message);
    }		
	}

	public void property_changed_cb(string interface_source, HashTable<string, Value?> changed_properties, string[] invalid )
	{	
		debug("properties-changed for interface %s and owner %s", interface_source, this.owner.name.down());
    
		if(changed_properties == null || interface_source.has_prefix(this.root_interface) == false ){
			warning("Property-changed hash is null or this is an interface that doesn't concerns us");
			return;
		}
		Value? play_v = changed_properties.lookup("PlaybackStatus");
		if(play_v != null){
			string state = this.player.PlaybackStatus;		
			debug("new playback state = %s", state);			
			TransportMenuitem.state p = (TransportMenuitem.state)this.determine_play_state(state);
			(this.owner.custom_items[PlayerController.widget_order.TRANSPORT] as TransportMenuitem).change_play_state(p);			
		}
		
		Value? pos_v = changed_properties.lookup("Position");
		if(pos_v != null){
			int64 pos = pos_v.get_int64();
			debug("new position = %i", (int)pos);
		}

		Value? meta_v = changed_properties.lookup("Metadata");
		if(meta_v != null){
			GLib.HashTable<string, Value?> changed_updates = clean_metadata();	
			this.owner.custom_items[PlayerController.widget_order.METADATA].reset(MetadataMenuitem.attributes_format());
			this.owner.custom_items[PlayerController.widget_order.METADATA].update(changed_updates,
			                          																						 MetadataMenuitem.attributes_format());			
		}
	}

	private GLib.HashTable<string, Value?> clean_metadata()
	{ 
		GLib.HashTable<string, Value?> changed_updates = this.player.Metadata; 
    Value? artist_v = this.player.Metadata.lookup("xesam:artist");
    if(artist_v != null){
		  string[] artists = (string[])this.player.Metadata.lookup("xesam:artist");
		  string display_artists = string.joinv(", ", artists);
		  changed_updates.replace("xesam:artist", display_artists);
		  debug("artist : %s", display_artists);
    }
    Value? length_v = this.player.Metadata.lookup("mpris:length");
    if(length_v != null){
		  int64 duration = this.player.Metadata.lookup("mpris:length").get_int64();
		  changed_updates.replace("mpris:length", duration/1000000); 
    }
		return changed_updates;
	}
	
	
	private TransportMenuitem.state determine_play_state(string status){
		if(status == null)
			return TransportMenuitem.state.PAUSED;
		
		if(status != null && status == "Playing"){
			debug("determine play state - state = %s", status);
			return TransportMenuitem.state.PLAYING;
		}
		return TransportMenuitem.state.PAUSED;		
	}
	
	public void initial_update()
	{
		TransportMenuitem.state update;
		if(this.player.PlaybackStatus == null){
			update = TransportMenuitem.state.PAUSED;
		}
		else{
			update = determine_play_state(this.player.PlaybackStatus);
		}
		debug("initial update - play state %i", (int)update);
		
		(this.owner.custom_items[PlayerController.widget_order.TRANSPORT] as TransportMenuitem).change_play_state(update);
		GLib.HashTable<string, Value?> cleaned_metadata = this.clean_metadata();
		this.owner.custom_items[PlayerController.widget_order.METADATA].update(cleaned_metadata,
			                          MetadataMenuitem.attributes_format());
	}

	public void transport_update(TransportMenuitem.action command)
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
				error("Exception thrown while calling function Raise - %s", e.message);
			}
		}
	}
}



