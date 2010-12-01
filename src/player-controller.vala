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

using Dbusmenu;
using Gee;

public class PlayerController : GLib.Object
{
	public const int WIDGET_QUANTITY = 4;

	public static enum widget_order{
		SEPARATOR,
		TITLE,
		METADATA,
		TRANSPORT,
	}

	public enum state{
	 	OFFLINE,
		INSTANTIATING,
		READY,
		CONNECTED,
		DISCONNECTED
	}
	
	public int current_state = state.OFFLINE;
		
	private Dbusmenu.Menuitem root_menu;
	public string name { get; set;}	
	public string mpris_name { get; set;}	  
	public ArrayList<PlayerItem> custom_items;	
	public Mpris2Controller mpris_bridge;
	public AppInfo? app_info { get; set;}
	public int menu_offset { get; set;}
  public string icon_name { get; set; }
		
	public PlayerController(Dbusmenu.Menuitem root,
                          GLib.AppInfo app,
                          string mpris_name,
                          string icon_name,
	                        int offset,
	                        state initial_state)
	{
		this.root_menu = root;
    this.app_info = app;
		this.name = format_player_name(this.app_info.get_name());
    this.mpris_name = mpris_name;
    this.icon_name = icon_name;
		this.custom_items = new ArrayList<PlayerItem>();
		this.current_state = initial_state;
		this.menu_offset = offset;
		construct_widgets();
		establish_mpris_connection();
		this.update_layout();   
	}

	public void update_state(state new_state)
	{
		debug("update_state - player controller %s : new state %i", this.name, new_state);
		this.current_state = new_state;
		this.update_layout();
	}
	
	public void activate()
	{
		this.establish_mpris_connection();	
	}

	/*
	 instantiate()
	 The user should be able to start the app from the transport bar when in an offline state
	 There is a need to wait before the application is on DBus before attempting to access its mpris address
	 Hence only when the it has registered with us via libindicate do we attempt to kick off mpris communication
	 */
	public void instantiate()
	{
		debug("instantiate in player controller for %s", this.name);
		try{
 	  	this.app_info.launch(null, null);
			this.update_state(state.INSTANTIATING);
		}
		catch(GLib.Error error){
			warning("Failed to launch app %s with error message: %s", this.name, error.message);
		}
	}
	
	private void establish_mpris_connection()
	{		
		if(this.current_state != state.READY){
			debug("establish_mpris_connection - Not ready to connect");
			return;
		}		
		this.mpris_bridge = new Mpris2Controller(this); 
		this.determine_state();
	}
	
	public void vanish()
	{
		foreach(Dbusmenu.Menuitem item in this.custom_items){
			root_menu.child_delete(item);			
		}
	}

	public void hibernate()
	{
		update_state(PlayerController.state.OFFLINE);
		this.custom_items[widget_order.TRANSPORT].reset(TransportMenuitem.attributes_format());
		this.custom_items[widget_order.METADATA].reset(MetadataMenuitem.attributes_format());
		TitleMenuitem title = this.custom_items[widget_order.TITLE] as TitleMenuitem;
		title.toggle_active_triangle(false);					
	}

	public void update_layout()
	{			
		if(this.current_state != state.CONNECTED){
			this.custom_items[widget_order.TRANSPORT].property_set_bool(MENUITEM_PROP_VISIBLE,
			                                                            false);
			this.custom_items[widget_order.METADATA].property_set_bool(MENUITEM_PROP_VISIBLE,
			                                                           false);
			return;	
		}
		this.custom_items[widget_order.METADATA].property_set_bool(MENUITEM_PROP_VISIBLE,
			                                                        this.custom_items[widget_order.METADATA].populated(MetadataMenuitem.attributes_format()));		
		this.custom_items[widget_order.TRANSPORT].property_set_bool(MENUITEM_PROP_VISIBLE,
		                                                            true);
	}
		
	private void construct_widgets()
	{
		// Separator item
		this.custom_items.add(new PlayerItem(CLIENT_TYPES_SEPARATOR));

		// Title item
		TitleMenuitem title_menu_item = new TitleMenuitem(this);
		this.custom_items.add(title_menu_item);

		// Metadata item
		MetadataMenuitem metadata_item = new MetadataMenuitem();
		this.custom_items.add(metadata_item);

		// Transport item
		TransportMenuitem transport_item = new TransportMenuitem(this);
		this.custom_items.add(transport_item);
				
		foreach(PlayerItem item in this.custom_items){
			root_menu.child_add_position(item, this.menu_offset + this.custom_items.index_of(item));			
		}
	}	
	
  private static string format_player_name(owned string app_info_name)
  {
    string result = app_info_name.down().strip();
    var tokens = result.split(" ");
    if(tokens.length > 1){
      result = tokens[0];
    }
    if(result.length > 1){
			result = result.up(1).concat(result.slice(1, result.length));
			debug("PlayerController->format_player_name - : %s", result);
		}		
    return result;
  }

	public void determine_state()
	{
		if(this.mpris_bridge.connected() == true){
			this.update_state(state.CONNECTED);
			TitleMenuitem title = this.custom_items[widget_order.TITLE] as TitleMenuitem;
			title.toggle_active_triangle(true);	
      this.mpris_bridge.initial_update();
		}
		else{
			this.update_state(state.DISCONNECTED);
		}
	} 
}
