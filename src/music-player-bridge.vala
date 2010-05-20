using Indicate;
using Dbusmenu;
using Gee;

public class MusicPlayerBridge : GLib.Object
{

  private Listener listener;
  private Dbusmenu.Menuitem root_menu;
	private HashMap<string, Dbusmenu.Menuitem> registered_clients;  
	
  public MusicPlayerBridge()
  {
		registered_clients = new HashMap<string, Dbusmenu.Menuitem> ();
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
    debug("MusicPlayerBridge -> set_root_menu_item");
		root_menu = menu;
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

  public void on_server_added(Indicate.ListenerServer object, string type)
  {
    debug("MusicPlayerBridge -> on_server_added with value %s", type);
		if(server_is_not_of_interest(type)) return;
		string client_name = type.split(".")[1];
		if (root_menu != null && client_name != null){
			Dbusmenu.Menuitem client_item = new Dbusmenu.Menuitem();
			client_item.property_set(MENUITEM_PROP_LABEL, client_name.concat(" is registered"));
			registered_clients.set(client_name, client_item); 
			root_menu.child_append(client_item);
			debug("client of name %s has successfully registered with us", client_name);
		}
  }

  public void on_server_removed(Indicate.ListenerServer object, string type)
  {
    debug("MusicPlayerBridge -> on_server_removed with value %s", type);
		if(server_is_not_of_interest(type)) return;
		string client_name = type.split(".")[1];
		if (root_menu != null && client_name != null){
			root_menu.child_delete(registered_clients[client_name]);
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

}




