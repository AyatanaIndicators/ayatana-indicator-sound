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

using Dbusmenu;
using Gee;
using GLib;

public class MusicPlayerBridge : GLib.Object
{
  private Dbusmenu.Menuitem root_menu;
  private HashMap<string, PlayerController> registered_clients;  
  private FamiliarPlayersDB playersDB;
  private Mpris2Watcher watcher;
  private const string DESKTOP_PREFIX = "/usr/share/applications/";

  public MusicPlayerBridge()
  {
    playersDB = new FamiliarPlayersDB();
    registered_clients = new HashMap<string, PlayerController> ();
  }
  
  private void try_to_add_inactive_familiar_clients(){
    foreach(string app in this.playersDB.records()){
      if(app == null){
        warning("App string in keyfile is null therefore moving on to next player");
        continue;
      }

      debug("attempting to make an app info from %s", app);

      DesktopAppInfo info = new DesktopAppInfo.from_filename(app);

      if(info == null){
        warning("Could not create a desktopappinfo instance from app,: %s , moving on to the next client", app);
        continue;
      }
      
      GLib.AppInfo app_info = info as GLib.AppInfo;
      var mpris_key = determine_key(app);
      PlayerController ctrl = new PlayerController(this.root_menu, 
                                                   app_info,
                                                   mpris_key,
                                                   playersDB.fetch_icon_name(app),
                                                   calculate_menu_position(),
                                                   PlayerController.state.OFFLINE);
      this.registered_clients.set(mpris_key, ctrl);
      }
  }
  
  private int calculate_menu_position()
  {
    if(this.registered_clients.size == 0){
      return 2;
    }
    else{
      return (2 + (this.registered_clients.size * PlayerController.WIDGET_QUANTITY));
    }
  }

	/*public void on_server_added(Indicate.ListenerServer object, string type)
  {
    debug("MusicPlayerBridge -> on_server_added with value %s", type);
		if(server_is_not_of_interest(type)) return;
		if ( this.root_menu != null ){
			listener_get_server_property_cb cb = (listener_get_server_property_cb)desktop_info_callback;
			this.listener.server_get_desktop(object, cb, this);					
		}
  }*/

  public void  client_has_become_available ( string desktop_file_name )
  {
    debug ( "client_has_become_available %s", desktop_file_name );
    string path = DESKTOP_PREFIX.concat ( desktop_file_name.concat( ".desktop" ) );    
    AppInfo? app_info = create_app_info ( path );
    if ( app_info == null ){
      warning ( "Could not create app_info for path %s \n Getting out of here ", path);
      return;
    }
    
    var mpris_key = determine_key ( desktop_file_name );

    if ( this.playersDB.already_familiar ( path ) == false ){
      debug("New client has registered that we have not seen before: %s", desktop_file_name );
      this.playersDB.insert ( path );
      PlayerController ctrl = new PlayerController ( this.root_menu,
                                                     app_info,
                                                     mpris_key,
                                                     playersDB.fetch_icon_name(path),                                                    
                                                     this.calculate_menu_position(),
                                                     PlayerController.state.READY );
      this.registered_clients.set ( mpris_key, ctrl );        
      debug ( "successfully created appinfo and instance from path and set it on the respective instance" );				
      }
      else{
        this.registered_clients[mpris_key].update_state ( PlayerController.state.READY );
        this.registered_clients[mpris_key].activate ( );      
        debug("Ignoring desktop file path callback because the db cache file has it already: %s \n", path);
      }
    }
  
  /*public void on_server_removed(Indicate.ListenerServer object, string type)
  {
    debug("MusicPlayerBridge -> on_server_removed with value %s", type);
		if(server_is_not_of_interest(type)) return;
		if (root_menu != null){
      var tmp = type.split(".");
      debug("attempt to remove %s", tmp[tmp.length-1]);
      if(tmp.length > 0){
			  registered_clients[tmp[tmp.length - 1]].hibernate();
			  debug("Successively offlined client %s", tmp[tmp.length - 1]);       
      }
		}
	}*/
  
  public void set_root_menu_item(Dbusmenu.Menuitem menu)
  {
    this.root_menu = menu;
    this.try_to_add_inactive_familiar_clients();
    this.watcher = new Mpris2Watcher ();
    this.watcher.clientappeared += this.client_has_become_available;
    this.watcher.test_signal_emission();
  }

  public static AppInfo? create_app_info ( string path )
  {
    DesktopAppInfo info = new DesktopAppInfo.from_filename ( path ) ;
    if ( path == null || info == null ){
      warning ( "Could not create a desktopappinfo instance from app: %s", path );
      return null;
    }
    GLib.AppInfo app_info = info as GLib.AppInfo;
    return app_info;
  }

  private static string? determine_key(owned string name)
  {
    string result = name;
    var temp = name.split("-");
    if (temp.length > 1){
      result = temp[0];
    }
    debug("determine key result = %s", result);
    return result;        
  }
  
}




