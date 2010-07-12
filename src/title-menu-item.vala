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
using DbusmenuTitle;
using Gee;

public class TitleMenuitem : PlayerItem
{
	public TitleMenuitem(PlayerController parent)
	{
		Object(item_type: MENUITEM_TYPE, owner: parent);
		this.property_set(MENUITEM_TEXT_NAME, parent.name);		
	}

	public override void handle_event(string name, GLib.Value input_value, uint timestamp)
	{
		debug("handle_event for owner %s with owner state = %i and title menu name %s", this.owner.name, this.owner.current_state, property_get(MENUITEM_TEXT_NAME));
		
		if(this.owner.current_state == PlayerController.state.OFFLINE)
		{
			this.owner.instantiate();
		}
	}
	

	public static HashSet<string> attributes_format()
	{
		HashSet<string> attrs = new HashSet<string>();		
		attrs.add(MENUITEM_TEXT_NAME);
 		return attrs;
	}	
}