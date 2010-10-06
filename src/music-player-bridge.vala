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

using Indicate;
using Dbusmenu;
using Gee;
using GLib;

public class MusicPlayerBridge : GLib.Object
{

  private Listener listener;
  private Dbusmenu.Menuitem root_menu;
	private HashMap<string, PlayerController> registered_clients;  
	private FamiliarPlayersDB playersDB;
	
  public MusicPlayerBridge()
  {
		playersDB = new FamiliarPlayersDB();
		registered_clients = new HashMap<string, PlayerController> ();
    listener = Listener.ref_default();
    listener.indicator_added += on_indicator_added;
    listener.indicator_removed.connect(on_indicator_removed);
    listener.indicator_modified.connect(on_indicator_modified);
    listener.server_added.connect(on_server_added);
    listener.server_removed.connect(on_server_removed);
    listener.server_count_changed.connect(on_server_count_changed);
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
				warning("Could not create a desktopappinfo instance from app: %s", app);
				continue;					
			}
			GLib.AppInfo app_info = info as GLib.AppInfo;
      // TODO refactor to remove need for further name refactoring in the player controller
      // truncate should not do a down() on the name
			PlayerController ctrl = new PlayerController(this.root_menu, 
					                                         truncate_player_name(app_info.get_name()),
					                                         calculate_menu_position(),
					                                         PlayerController.state.OFFLINE);
			ctrl.app_info = app_info;
      if(ctrl.app_info == null)
        warning("for some reason the app info is null");
      
			this.registered_clients.set(truncate_player_name(app_info.get_name()), ctrl);					
		}
	}

  private static string truncate_player_name(string app_info_name)
  {
    string result = app_info_name.down().strip();

    var tokens = result.split(" ");

    if(tokens.length > 1){
      result = tokens[0];
    }
    debug("truncate player name %s", result);
    return result;
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
	
	public void on_server_added(Indicate.ListenerServer object, string type)
  {
    debug("MusicPlayerBridge -> on_server_added with value %s", type);
		if(server_is_not_of_interest(type)) return;
		if ( this.root_menu != null ){
			listener_get_server_property_cb cb = (listener_get_server_property_cb)desktop_info_callback_on_addition;
			this.listener.server_get_desktop(object, cb, this);					
		}
  }

  private void desktop_info_callback_on_addition ( Indicate.ListenerServer server,
	                                 	                owned string path,
                                                    void* data )                                                  	                                  
	{
		MusicPlayerBridge bridge = data as MusicPlayerBridge;
		AppInfo? app_info = create_app_info(path);
    var name = truncate_player_name(app_info.get_name());
		if(path.contains("/") && bridge.playersDB.already_familiar(path) == false){
			debug("About to store desktop file path: %s", path);
			bridge.playersDB.insert(path);
			PlayerController ctrl = new PlayerController(bridge.root_menu,
			                                             name,
			                                             bridge.calculate_menu_position(),
			                                             PlayerController.state.READY);
			ctrl.set("app_info", app_info);
      bridge.registered_clients.set(name, ctrl);        
      debug("successfully created appinfo and instance from path and set it on the respective instance");				
		}
		else{
		  bridge.registered_clients[name].update_state(PlayerController.state.READY);
			bridge.registered_clients[name].activate();      
			debug("Ignoring desktop file path callback because the db cache file has it already: %s", path);
		}
	}
  
  public void on_server_removed(Indicate.ListenerServer object, string type)
  {
    debug("MusicPlayerBridge -> on_server_removed with value %s", type);
		if(server_is_not_of_interest(type)) return;
		if (root_menu != null){
			listener_get_server_property_cb cb = (listener_get_server_property_cb)desktop_info_callback_on_removal;
			this.listener.server_get_desktop(object, cb, this);					
		}
	}

  private void desktop_info_callback_on_removal ( Indicate.ListenerServer server,
	                                 	              owned string path,
                                                  void* data )	                                  
  {
		MusicPlayerBridge bridge = data as MusicPlayerBridge;
		AppInfo? app_info = create_app_info(path);
    var name = truncate_player_name(app_info.get_name());
	  registered_clients[name].hibernate();
		debug("Successively offlined client %s", name);    
  }
  
	private bool server_is_not_of_interest(string type){
    if (type == null) return true;
    if (type.contains("music") == false) {
      debug("server is of no interest,  it is not an music server");
      return true;
    }
		return false;
	}
		
  public void set_root_menu_item(Dbusmenu.Menuitem menu)
  {
		this.root_menu = menu;
		try_to_add_inactive_familiar_clients();
  }

	public void on_server_count_changed(Indicate.ListenerServer object, uint i)
  {
    debug("MusicPlayerBridge-> on_server_count_changed with value %u", i);
  }
  public void on_indicator_added(ListenerServer object, ListenerIndicator p0)
  {
    debug("MusicPlayerBridge-> on_indicator_added");
  }

  public void on_indicator_removed(Indicate.ListenerServer object, Indicate.ListenerIndicator p0)
  {
    debug("MusicPlayerBridge -> on_indicator_removed");
  }

  public void on_indicator_modified(Indicate.ListenerServer object, Indicate.ListenerIndicator p0, string s)
  {
    debug("MusicPlayerBridge -> indicator_modified with vale %s", s );
  }

	public static AppInfo? create_app_info(string path)
	{
		DesktopAppInfo info = new DesktopAppInfo.from_filename(path);
		if(path == null){
			warning("Could not create a desktopappinfo instance from app: %s", path);
			return null;
		}
		GLib.AppInfo app_info = info as GLib.AppInfo;		
		return app_info;
	}

}




