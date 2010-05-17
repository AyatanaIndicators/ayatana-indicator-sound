using DbusmenuGlib;

public class RhythmboxController : GLib.Object
{
  private DBus.Connection connection;
  private dynamic DBus.Object rhythmbox_player;
  private dynamic DBus.Object rhythmbox_shell;
  private dynamic DBus.Object rhythmbox_playlistmgr;

  public RhythmboxController()
  {
    try {
      this.connection = DBus.Bus.get (DBus.BusType.SESSION);
    } catch (Error e) {
      debug("Problems connecting to the session bus - %s", e.message);
    }

    this.rhythmbox_player = this.connection.get_object ("org.gnome.Rhythmbox", "/org/gnome/Rhythmbox/Player", "org.gnome.Rhythmbox.Player");
    this.rhythmbox_shell = connection.get_object ("org.gnome.Rhythmbox", "/org/gnome/Rhythmbox/Shell", "org.gnome.Rhythmbox.Shell");
    this.rhythmbox_playlistmgr = connection.get_object ("org.gnome.Rhythmbox", "/org/gnome/Rhythmbox/PlaylistManager", "/org/gnome/Rhythmbox/PlaylistManager");

    this.rhythmbox_player.playingUriChanged += onUriChange;
    this.rhythmbox_player.elapsedChanged += onElapsedChange;
		
    debug("New rhythmbox controller has been instantiated");
  }

  private void onUriChange(dynamic DBus.Object rhythmbox, string uri)
  {
    debug("onUriChange, new uri : %s", uri);
		//;// = new HashTable<string, string>(str_hash,str_equal);
		HashTable<string,Value?> ht = this.rhythmbox_shell.getSongProperties(uri);
		var l = ht.get_keys();
		foreach(string s in l){
			debug("key = %s", s);		
		}
  }

  private void onElapsedChange(dynamic DBus.Object rhythmbox, uint32 time)
  {
    debug("onElapsedChange, new time = %u", time);
  }

}
