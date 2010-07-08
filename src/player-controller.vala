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
	public const int METADATA = 2;	
	private const int TRANSPORT = 3;

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
	public ArrayList<PlayerItem> custom_items;	
	public MprisController mpris_adaptor;
	public AppInfo? app_info { get; set;}
		
	public PlayerController(Dbusmenu.Menuitem root, string client_name, state initial_state)
	{
		this.root_menu = root;
		this.name = format_client_name(client_name.strip());
		this.custom_items = new ArrayList<PlayerItem>();
		this.update_state(initial_state);
		construct_widgets();
		establish_mpris_connection();
		update_layout();
	}

	public void update_state(state new_state)
	{
		debug("update_state : new state %i", new_state);
		this.current_state = new_state;
	}
	
	public void activate()
	{
		this.establish_mpris_connection();	
		this.custom_items[METADATA].property_set_bool(MENUITEM_PROP_VISIBLE, true);		
	}

	/*
	 instantiate()
	 The user should be able to start the app from the transport bar when in an offline state
	 There is a need to wait before the application is on DBus before attempting to access its mpris address
	 Hence only when the it has registered with us via libindicate do we attempt to kick off mpris communication
	 */
	public void instantiate()
	{
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
		if(this.name == "Vlc"){
			this.mpris_adaptor = new MprisControllerV2(this.name, this);
		}
		else{
			this.mpris_adaptor = new MprisController(this.name, this);
		}
		if(this.mpris_adaptor.connected() == true){
			this.update_state(state.CONNECTED);
		}
		else{
			this.update_state(state.DISCONNECTED);
		}
		this.update_layout();
	}
	
	public void vanish()
	{
		foreach(Dbusmenu.Menuitem item in this.custom_items){
			root_menu.child_delete(item);			
		}
	}

	private void update_layout()
	{
		bool visibility = true;
		if(this.current_state != state.CONNECTED){
			visibility = false;
		}
		debug("about the set the visibility on both the transport and metadata widget to %s", visibility.to_string());
		this.custom_items[TRANSPORT].property_set_bool(MENUITEM_PROP_VISIBLE, visibility);
		this.custom_items[METADATA].property_set_bool(MENUITEM_PROP_VISIBLE, visibility);
	}
	
	
	private void construct_widgets()
	{
		// Separator item
		this.custom_items.add(new PlayerItem(CLIENT_TYPES_SEPARATOR));

		// Title item
		TitleMenuitem title_menu_item = new TitleMenuitem(this, this.name);
		this.custom_items.add(title_menu_item);

		// Metadata item
		MetadataMenuitem metadata_item = new MetadataMenuitem();
		this.custom_items.add(metadata_item);
		
		// Transport item
		TransportMenuitem transport_item = new TransportMenuitem(this);
		this.custom_items.add(transport_item);

		int offset = 2;
		foreach(PlayerItem item in this.custom_items){
			root_menu.child_add_position(item, offset + this.custom_items.index_of(item));			
		}
	}	
	
	private static string format_client_name(string client_name)
	{
		string formatted = client_name;
		if(formatted.len() > 1){
			formatted = client_name.up(1).concat(client_name.slice(1, client_name.len()));
			debug("PlayerController->format_client_name - : %s", formatted);
		}		
		return formatted;
	}

}