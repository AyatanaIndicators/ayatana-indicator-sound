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


[DBus (name = "org.mpris.MediaPlayer.Player")]
public interface MprisPlayer : Object {
	public struct Status { 
    public int32 Playback_State;
    public double Playback_Rate;
    public bool Repeat_State;
    public bool Shuffle_State;
		public bool Endless_State;
	}
	public abstract void PlayPause() throws DBus.Error;
	public abstract void Pause() throws DBus.Error;
	public abstract void Next() throws DBus.Error;
	public abstract void Previous() throws DBus.Error;

	public abstract signal void StatusChanged(Status update);
}

/*
 This class will entirely replace mpris-controller.vala hence why there is no
 point in trying to get encorporate both into the same object model. 
 */
public class Mpris2Controller : GLib.Object
{		
	private DBus.Connection connection;
	public dynamic DBus.Object mpris2_root {get; construct;}		
	public MprisPlayer mpris2_player {get; construct;}		
	public PlayerController owner {get; construct;}	
	
	public Mpris2Controller(PlayerController ctrl)
	{
		Object(owner: ctrl);				
	}
	
	construct{
    try {
			debug("going to create this mpris 2 controller");			
      this.connection = DBus.Bus.get (DBus.BusType.SESSION);
    } catch (Error e) {
      error("Problems connecting to the session bus - %s", e.message);
    }		
		this.mpris2_root = this.connection.get_object ("org.mpris.mediaplayers.".concat(this.owner.name.down()),
		                                              "/org/mpris/MediaPlayer", 
		                                              "org.mpris.MediaPlayer");				

		this.mpris2_player = (MprisPlayer)this.connection.get_object ("org.mpris.mediaplayers.".concat(this.owner.name.down()) ,
		                                                							"/org/mpris/MediaPlayer/Player", 
		                                                							"org.mpris.MediaPlayer.Player");			
		//this.mpris2_player.TrackChange += onTrackChange;	
    this.mpris2_player.StatusChanged += onStatusChanged;
		initial_update();
	}
	
	private void initial_update()
	{
		//status st = this.mpris2_player.Status;
		//unowned ValueArray ar = (ValueArray)st;
		//unowned ValueArray ar = (ValueArray)this.mpris2_player.Status;						
		bool r  =  (bool)this.mpris2_player.Status.Shuffle_State;
		int32 p  =  (int32)this.mpris2_player.Status.Playback_State;

		debug("initial update - play state %i", p);
		debug("initial update - shuffle state %s", r.to_string());
		
		/*(this.owner.custom_items[PlayerController.widget_order.TRANSPORT] as TransportMenuitem).change_play_state(play_state);
		this.owner.custom_items[PlayerController.widget_order.METADATA].update(this.mpris2_player.GetMetadata(),
			                          MetadataMenuitem.attributes_format());
		this.owner.custom_items[PlayerController.widget_order.SCRUB].update(this.mpris2_player.GetMetadata(),
			                      ScrubMenuitem.attributes_format());		
		ScrubMenuitem scrub = this.owner.custom_items[PlayerController.widget_order.SCRUB] as ScrubMenuitem;
		scrub.update_position(this.mpris2_player.PositionGet());
		*/
	}

	public void transport_event(TransportMenuitem.action command)
	{		
		debug("transport_event input = %i", (int)command);
		if(command == TransportMenuitem.action.PLAY_PAUSE){
			debug("transport_event PLAY_PAUSE");
			this.mpris2_player.PlayPause();							
		}
		else if(command == TransportMenuitem.action.PREVIOUS){
			this.mpris2_player.Previous();
		}
		else if(command == TransportMenuitem.action.NEXT){
			this.mpris2_player.Next();
		}		
	}

	public void set_position(double position)
	{
		/*
		debug("Set position with pos (0-100) %f", position);
		HashTable<string, Value?> data = this.mpris2_player.GetMetadata();
		Value? time_value = data.lookup("time");
		if(time_value == null){
			warning("Can't fetch the duration of the track therefore cant set the position");
			return;
		}
		uint32 total_time = time_value.get_uint();
		debug("total time of track = %i", (int)total_time);				
		double new_time_position = total_time * position/100.0;
		debug("new position = %f", (new_time_position * 1000));		
		this.mpris2_player.SetPosition((int32)(new_time_position));
		ScrubMenuitem scrub = this.owner.custom_items[PlayerController.widget_order.SCRUB] as ScrubMenuitem;
		scrub.update_position(this.mpris2_player.PositionGet());				
		*/
	}
	
	public bool connected()
	{
		return (this.mpris2_player != null);
	}
	
	private void onStatusChanged(MprisPlayer.Status st)
  {
		debug("on status changed - fucking jesus mother of god");
		debug("new playback state = %i", st.Playback_State);
		/*
		debug("onStatusChange - signal received");
		status* status = &st;
		unowned ValueArray ar = (ValueArray)status;		
		int play_state = ar.get_nth(0).get_int();
		debug("onStatusChange - play state %i", play_state);
		HashTable<string, Value?> ht = new HashTable<string, Value?>(str_hash, str_equal);
		Value v = Value(typeof(int));
		v.set_int(play_state);
		ht.insert("state", v); 
		this.owner.custom_items[PlayerController.widget_order.TRANSPORT].update(ht, TransportMenuitem.attributes_format());
		this.owner.custom_items[PlayerController.widget_order.SCRUB].update(ht, ScrubMenuitem.attributes_format());
		*/
	}
	
	private void onTrackChange(dynamic DBus.Object mpris_client, HashTable<string,Value?> ht)
	{
		/*(debug("onTrackChange");
		this.owner.custom_items[PlayerController.widget_order.METADATA].reset(MetadataMenuitem.attributes_format());
		this.owner.custom_items[PlayerController.widget_order.SCRUB].reset(ScrubMenuitem.attributes_format());
		this.owner.custom_items[PlayerController.widget_order.METADATA].update(ht,
		                            MetadataMenuitem.attributes_format());
		debug("about to update the duration on the scrub bar");
		this.owner.custom_items[PlayerController.widget_order.SCRUB].update(this.mpris2_player.GetMetadata(),
		                        ScrubMenuitem.attributes_format());		
		ScrubMenuitem scrub = this.owner.custom_items[PlayerController.widget_order.SCRUB] as ScrubMenuitem;
		scrub.update_position(this.mpris2_player.PositionGet());
		*/
	}
}



