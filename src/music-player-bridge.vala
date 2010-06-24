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

  public void set_root_menu_item(Dbusmenu.Menuitem menu)
  {
		root_menu = menu;
  }

//static void 
//desktop_cb (IndicateListener * listener, IndicateListenerServer * server, gchar * value, gpointer data)

	public void desktop_info_callback(Indicate.Listener listener,
	                                 	Indicate.ListenerServer server,
	                                 	owned string value, void* data)	                                  
	{
		
	}
	
	public void on_server_added(Indicate.ListenerServer object, string type)
  {
    debug("MusicPlayerBridge -> on_server_added with value %s", type);
		if(server_is_not_of_interest(type)) return;
		string client_name = type.split(".")[1];
		listener_get_server_property_cb cb = (listener_get_server_property_cb)desktop_info_callback;
		this.listener.server_get_desktop(object, cb);
		
		if (root_menu != null && client_name != null){
			PlayerController ctrl = new PlayerController(root_menu, client_name, true);
			registered_clients.set(client_name, ctrl); 
			debug("client of name %s has successfully registered with us", client_name);
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
			debug("Successively removed menu_item for client %s from registered_clients", client_name);
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

}




