using Dbusmenu;
using Gee;

public class TransportMenuItem : Dbusmenu.Menuitem
{
	/* Not ideal duplicate definition of const - see common-defs/h */
 	const string DBUSMENU_TRANSPORT_MENUITEM_TYPE = "x-canonical-transport-bar";

	public TransportMenuItem()
  {
		this.property_set(MENUITEM_PROP_TYPE, DBUSMENU_TRANSPORT_MENUITEM_TYPE); 			
	}

	public override void handle_event(string name, GLib.Value input_value, uint timestamp)
	{
		debug("TransportItem -> handle event caught!");
	}
	
}