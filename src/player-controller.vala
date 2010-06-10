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
		this.name = format_client_name(client_name);
		this.is_active = active;
		this.custom_items = new ArrayList<Dbusmenu.Menuitem>();
			//Dbusmenu.Menuitem[];
		self_construct();
	}

	public void self_construct()
	{
		Dbusmenu.Menuitem client_item = new Dbusmenu.Menuitem();
		this.custom_items.add(client_item);
		client_item.property_set(MENUITEM_PROP_LABEL, this.name.concat(""));			
		TransportMenuItem transport_item = new TransportMenuItem();			
		this.custom_items.add(transport_item);
		root_menu.child_append(client_item);
		root_menu.child_append(transport_item);		
	}

	public void vanish()
	{
		foreach(Dbusmenu.Menuitem item in this.custom_items){
			root_menu.child_delete(item);			
		}
	}
	
	public static string format_client_name(string client_name)
	{
		debug("PlayerController->format_client_name");
		//string first_letter = client_name.slice(1);
		//debug("PlayerController->format_client_name - first_letter: %s", first_letter);
		return client_name;
	}
		
}