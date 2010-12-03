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

// TODO: Until match rules are properly supported in vala we need to explicitly
//       watch for each client inorder to facilitate proper registration.

public class Mpris2Watcher : GLib.Object
{
  const string BANSHEE_BUS_NAME = "org.mpris.MediaPlayer2.banshee";
  private DBusConnection connection;
  public signal void clientappeared ( string desktop_name );

  public Mpris2Watcher ()
  {
  }
  construct
  {  
    try {
      this.connection = Bus.get_sync ( BusType.SESSION );
      GLib.BusNameAppearedCallback banshee_change_cb = (GLib.BusNameAppearedCallback)banshee_appeared;
      GLib.BusNameVanishedCallback banshee_gone_cb = (GLib.BusNameVanishedCallback)banshee_disappeared;
      Bus.watch_name_on_connection ( this.connection, 
                                     BANSHEE_BUS_NAME,
                                     BusNameWatcherFlags.AUTO_START,                      
                                     banshee_change_cb,
                                     banshee_gone_cb );
    }
    catch ( IOError e ){
      warning( "Mpris2watcher could not set up a watch for mpris clients appearing on the bus: %s",
               e.message );
    }
  }

  private void banshee_appeared ( GLib.DBusConnection connection,
                                  string name,
                                  string name_owner)
  {
    try {
		  MprisRoot mpris2_root = Bus.get_proxy_sync (  BusType.SESSION,
                                                    BANSHEE_BUS_NAME,
			                                              "/org/mpris/MediaPlayer2" );
      //this.the_bridge.client_has_become_available ( mpris2_root.DesktopEntry );
      debug ( "On Name Appeared - name %s, name_owner %s",
               mpris2_root.DesktopEntry,
               name_owner );
      debug ( "this pointer in banshee appeared callback %i", (int)this);
      this.clientappeared ( "mpris2_root.DesktopEntry" );
    }
    catch ( IOError e ){
      warning( "Mpris2watcher could not instantiate an mpris root for banshee: %s",
               e.message );
    }    
  }

  public void test_signal_emission()
  {
    this.clientappeared ( "test signal emission" );
    debug ( "this pointer in test-signal-emission %i", (int)this);
  }
  
  private void banshee_disappeared ( GLib.DBusConnection connection,
                                     string name,
                                     string name_owner )
  {
    debug ( "On Name Disappeared - name %s, name_owner %s", name, name_owner );
  }
  
}