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
    listener.indicator_added.connect(on_indicator_added);
    listener.indicator_removed.connect(on_indicator_removed);
    listener.indicator_modified.connect(on_indicator_modified);
    listener.server_added.connect(on_server_added);
    listener.server_removed.connect(on_server_removed);
    listener.server_count_changed.connect(on_server_count_changed);
  }

	private void try_to_add_inactive_familiar_clients(){
		// TODO handle multple players - just working with one right now
		int count = 0;
		foreach(string app in this.playersDB.records()){
			if(count == 0){
				if(app == null){
					warning("App string in keyfile is null therefore moving on to next player");
					continue;
				}
				DesktopAppInfo info = new DesktopAppInfo.from_filename(app); 
				if(info == null){
					warning("Could not create a desktopappinfo instance from app: %s", app);
					continue;					
				}
				GLib.AppInfo app_info = info as GLib.AppInfo;
				PlayerController ctrl = new PlayerController(this.root_menu, 
				                                             app_info.get_name(),
				                                             calculate_menu_position(),
				                                             PlayerController.state.OFFLINE);
				ctrl.set("app_info", app_info);
				this.registered_clients.set(app_info.get_name().down().strip(), ctrl);					
				debug("Created a player controller for %s which was found in the cache file", app_info.get_name().down().strip());
				count += 1;					
			}
			break;
		}
	}

	private int calculate_menu_position()
	{
		if(this.registered_clients.size == 0){
			return 2;
		}
		else{
			return (2 + (this.registered_clients.size * 4));
		}
	}
	
	public void on_server_added(Indicate.ListenerServer object, string type)
  {
    debug("MusicPlayerBridge -> on_server_added with value %s", type);
		if(server_is_not_of_interest(type)) return;
		string client_name = type.split(".")[1];
		if (root_menu != null && client_name != null){
			// If we have an instance already for this player, ensure it is switched to active
			if(this.registered_clients.keys.contains(client_name)){
				debug("It figured out that it already has an instance for this player already");
				this.registered_clients[client_name].update_state(PlayerController.state.READY);
				this.registered_clients[client_name].activate();
			}
			//else init a new one
			else{			
				
				PlayerController ctrl = new PlayerController(root_menu,
				                                             client_name,
				                                             calculate_menu_position(),
				                                             PlayerController.state.READY);
				registered_clients.set(client_name, ctrl); 
				
				debug("New Client of name %s has successfully registered with us", client_name);
			}
			// irregardless check that it has a desktop file if not kick off a request for it
			if(this.registered_clients[client_name].app_info == null){
				listener_get_server_property_cb cb = (listener_get_server_property_cb)desktop_info_callback;
				this.listener.server_get_desktop(object, cb, this);					
			}			
		}
  }

  public void on_server_removed(Indicate.ListenerServer object, string type)
  {
    debug("MusicPlayerBridge -> on_server_removed with value %s", type);
		if(server_is_not_of_interest(type)) return;
		string client_name = type.split(".")[1];
		if (root_menu != null && client_name != null){
			registered_clients[client_name].vanish();
			registered_clients.remove(client_name);
			debug("Successively removed menu_item for client %s from registered_clients",
			      client_name);
		}
	}
	
	private bool server_is_not_of_interest(string type){
    if (type == null) return true;
    if (type.contains("music") == false) {
      debug("server is of no interest,  it is not an music server");
      return true;
    }
		return false;
	}
		
	private void desktop_info_callback(Indicate.ListenerServer server,
	                                 	owned string path, void* data)	                                  
	{
		MusicPlayerBridge bridge = data as MusicPlayerBridge;
		if(path.contains("/") && bridge.playersDB.already_familiar(path) == false){
			debug("About to store desktop file path: %s", path);
			bridge.playersDB.insert(path);
			AppInfo? app_info = create_app_info(path);
			if(app_info != null){
				PlayerController ctrl = bridge.registered_clients[app_info.get_name().down().strip()];				
				ctrl.set("app_info", app_info);
				debug("successfully created appinfo from path and set it on the respective instance");				
			}	
		}
		else{
			debug("Ignoring desktop file path because its either invalid of the db cache file has it already: %s", path);
		}
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
  public void on_indicator_added(Indicate.ListenerServer object, Indicate.ListenerIndicator p0)
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




