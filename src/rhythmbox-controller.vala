using DbusmenuGlib;

public class RhythmboxController : GLib.Object{
  private DBus.Connection connection;
  private dynamic DBus.Object rhythmbox_player;
  private dynamic DBus.Object rhythmbox_shell;
  private dynamic DBus.Object rhythmbox_playlistmgr;

	public RhythmboxController(){	
		try{
			this.connection = DBus.Bus.get (DBus.BusType.SESSION);
		}
		catch(Error e){
			debug("Problems connecting to the session bus - %s", e.message);			
		}
    
    this.rhythmbox_player = this.connection.get_object ("org.gnome.Rhythmbox", "/org/gnome/Rhythmbox/Player", "org.gnome.Rhythmbox.Player");
		this.rhythmbox_shell = connection.get_object ("org.gnome.Rhythmbox", "/org/gnome/Rhythmbox/Shell", "/org/gnome/Rhythmbox/Shell");
		this.rhythmbox_playlistmgr = connection.get_object ("org.gnome.Rhythmbox", "/org/gnome/Rhythmbox/PlaylistManager", "/org/gnome/Rhythmbox/PlaylistManager");

		//this.rhythmbox_player.PlayingUriChanged += onUriChange;
		this.rhythmbox_player.elapsedChanged += onElapsedChange;
		
		this.rhythmbox_player.setMute(false);
	  bool  b = this.rhythmbox_player.getMute();
		this.rhythmbox_player.playPause(true);
		//this.rhythmbox_playlistmgr.getPlaylists();
		//debug("playlist = %s", [0]);		
		debug("New rhythmbox controller has been instantiated %i", (int)b);
	}

//	private void onUriChange(dynamic DBus.Object rhythmbox, string uri){
//		debug("onUriChange, new uri : %s", uri);	
//	}

	private void onElapsedChange(dynamic DBus.Object rhythmbox, uint32 time){
		debug("onElapsedChange, new time = %u", time);
	}

}
