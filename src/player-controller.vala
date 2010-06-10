using Dbusmenu;
using Gee;

public class PlayerController : GLib.Object
{
	private Dbusmenu.Menuitem root_menu;
	private string name;
	private bool is_active;
	private ArrayList<Dbusmenu.Menuitem> custom_items;	
	
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
	
	private void self_construct()
	{
		Dbusmenu.Menuitem client_item = new Dbusmenu.Menuitem();
		this.custom_items.add(client_item);
		client_item.property_set(MENUITEM_PROP_LABEL, this.name.concat(""));			
		TransportMenuItem transport_item = new TransportMenuItem();			
		this.custom_items.add(transport_item);
		root_menu.child_append(client_item);
		root_menu.child_append(transport_item);		
	}

	private static string format_client_name(string client_name)
	{
		string formatted = client_name;
		//debug("PlayerController->format_client_name");
		if(formatted.len() > 1){
			formatted = client_name.up(1).concat(client_name.slice(1, client_name.len()));
			debug("PlayerController->format_client_name - : %s", formatted);
		}
		
		return formatted;
	}
	
}