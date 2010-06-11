using Dbusmenu;
using Gee;

public class TransportMenuitem : Dbusmenu.Menuitem
{
	/* Not ideal duplicate definition of const - see common-defs/h */
 	const string DBUSMENU_TRANSPORT_MENUITEM_TYPE = "x-canonical-transport-bar";
 	const string DBUSMENU_TRANSPORT_MENUITEM_STATE = "x-canonical-transport-state";

	public TransportMenuitem()
  {
		this.property_set(MENUITEM_PROP_TYPE, DBUSMENU_TRANSPORT_MENUITEM_TYPE);
		this.property_set(DBUSMENU_TRANSPORT_MENUITEM_STATE, "play");
	}

	public override void handle_event(string name, GLib.Value input_value, uint timestamp)
	{
		this.property_set(DBUSMENU_TRANSPORT_MENUITEM_STATE, "1");		
		debug("TransportItem -> handle event caught!");
	}	
}