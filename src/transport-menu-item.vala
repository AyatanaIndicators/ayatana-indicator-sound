/*
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
using DbusmenuTransport;

public class TransportMenuitem : PlayerItem
{
	
	public TransportMenuitem(PlayerController parent)
  {
		Object(item_type: MENUITEM_TYPE, owner: parent); 
	}

	public void change_play_state(int state)
	{
		this.property_set_int(MENUITEM_PLAY_STATE, state);	
	}
	
	public override void handle_event(string name, GLib.Value input_value, uint timestamp)
	{
		int input = input_value.get_int();
		debug("handle_event with value %s", input.to_string());
		if(input > 0){
			this.owner.mpris_adaptor.transport_event(input);
		}
		else{
			debug("A mouse event I'm not interested in");
		}
	}	

	public static HashSet<string> attributes_format()
	{
		HashSet<string> attrs = new HashSet<string>();		
		attrs.add(MENUITEM_PLAY_STATE);
		return attrs;
	}	
}