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

public class PlayerItem : Dbusmenu.Menuitem
{
	public MprisController mpris_adaptor;
	
	public PlayerItem()
	{
	}
	
	public virtual void update(HashMap<string, Value?> data)
	{
		debug("PlayerItem::update()");
		foreach(var key in this.attributes().keys){
			this.attributes.get(key);
		}
	}

	public void set_adaptor(MprisController adaptor)
	{
		this.mpris_adaptor = adaptor;		
	}

	// Bespoke constructors for player items
	// Title item
	public static PlayerItem new_title_item(dynamic string name)
	{
		PlayerItem item = new PlayerItem();
		item.property_set(MENUITEM_PROP_LABEL, name);					
		item.property_set(MENUITEM_PROP_ICON_NAME, "applications-multimedia");			
		return item;		
	}

	// Separator item
	public static PlayerItem new_separator_item()
	{
		PlayerItem separator = new PlayerItem();
		separator.property_set(MENUITEM_PROP_TYPE, CLIENT_TYPES_SEPARATOR);					
		return separator;
	}

	
}

