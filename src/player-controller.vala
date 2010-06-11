using Dbusmenu;
using Gee;

public class PlayerController : GLib.Object
{
	private Dbusmenu.Menuitem root_menu;
	private string name;
	private bool is_active;
	private ArrayList<Dbusmenu.Menuitem> custom_items;	

	// TODO: pass in the appropriate position for the menu (to handle multiple players)
	public PlayerController(Dbusmenu.Menuitem root, string client_name, bool active)
	{
		this.root_menu = root;
		this.name = format_client_name(client_name.strip());
		this.is_active = active;
		this.custom_items = new ArrayList<Dbusmenu.Menuitem>();
		self_construct();
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
		title_item.property_set(MENUITEM_PROP_LABEL, this.name.concat(""));					
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