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
	
	private Dbusmenu.Menuitem root_menu;
	private string name;
	private bool is_active;
	public ArrayList<PlayerItem> custom_items;	
	private MprisController mpris_adaptor;
	private string desktop_path;
	
	public PlayerController(Dbusmenu.Menuitem root, string client_name, bool active)
	{
		this.root_menu = root;
		this.name = format_client_name(client_name.strip());
		this.is_active = active;
		this.custom_items = new ArrayList<PlayerItem>();
		self_construct();
		
		// Temporary scenario to handle both v1 and v2 of MPRIS.
		if(this.name == "Vlc"){
			this.mpris_adaptor = new MprisControllerV2(this.name, this);
		}
		else{
			this.mpris_adaptor = new MprisController(this.name, this);
		}			
		this.custom_items[TRANSPORT].set_adaptor(this.mpris_adaptor);

		// At start up if there is no metadata then hide the item.
		// TODO: NOT working -> dbus menu bug ?
		//((MetadataMenuitem)this.custom_items[METADATA]).check_layout();
	}

	public void vanish()
	{
		foreach(Dbusmenu.Menuitem item in this.custom_items){
			root_menu.child_delete(item);			
		}
	}
	
	private bool self_construct()
	{
		// Separator item
		this.custom_items.add(PlayerItem.new_separator_item());

		// Title item
		this.custom_items.add(PlayerItem.new_title_item(this.name));

		// Metadata item
		MetadataMenuitem metadata_item = new MetadataMenuitem();
		this.custom_items.add(metadata_item);
		
		// Transport item
		TransportMenuitem transport_item = new TransportMenuitem();
		this.custom_items.add(transport_item);

		int offset = 2;
		foreach(PlayerItem item in this.custom_items){
			root_menu.child_add_position(item, offset + this.custom_items.index_of(item));			
		}
		return true;
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