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
	private const int METADATA = 2;
	private Dbusmenu.Menuitem root_menu;
	private string name;
	private bool is_active;
	private ArrayList<Dbusmenu.Menuitem> custom_items;	
	private MprisController mpris_adaptor;

	// TODO: pass in the appropriate position for the menu (to handle multiple players)
	public PlayerController(Dbusmenu.Menuitem root, string client_name, bool active)
	{
		this.root_menu = root;
		this.name = format_client_name(client_name.strip());
		this.is_active = active;
		this.custom_items = new ArrayList<Dbusmenu.Menuitem>();
		self_construct();
		// Temporary scenario to handle both v1 and v2 of MPRIS.
		if(this.name == "Vlc"){
			this.mpris_adaptor = new MprisControllerV2(this.name, this);
		}else{
			this.mpris_adaptor = new MprisController(this.name, this);
		}					
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
		Dbusmenu.Menuitem separator_item = new Dbusmenu.Menuitem();
		separator_item.property_set(MENUITEM_PROP_TYPE, CLIENT_TYPES_SEPARATOR);			
		this.custom_items.add(separator_item);

		// Title item
		Dbusmenu.Menuitem title_item = new Dbusmenu.Menuitem();
		title_item.property_set(MENUITEM_PROP_LABEL, this.name);					
		title_item.property_set(MENUITEM_PROP_ICON_NAME, "applications-multimedia");			
		this.custom_items.add(title_item);

		// Metadata item
		MetadataMenuitem metadata_item = new MetadataMenuitem();
		this.custom_items.add(metadata_item);
		
		// Transport item
		TransportMenuitem transport_item = new TransportMenuitem();
		this.custom_items.add(transport_item);

		int offset = 2;
		foreach(Dbusmenu.Menuitem item in this.custom_items){
			root_menu.child_add_position(item, offset + this.custom_items.index_of(item));			
		}
		return true;
	}	

	public void update_playing_info(HashMap<string, string> data)
	{
		debug("PlayerController - update_playing_info");
		MetadataMenuitem item = (MetadataMenuitem)this.custom_items[METADATA];		
		item.update(data);
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