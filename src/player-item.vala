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

public class PlayerItem : Dbusmenu.Menuitem
{
	public PlayerController owner {get; construct;}
	public string item_type { get; construct; }

	public PlayerItem(string type)
	{		
		Object(item_type: type);
	}
	
	construct {
		this.property_set(MENUITEM_PROP_TYPE, item_type);
	}

	public void reset(HashSet<string> attrs){		
		foreach(string s in attrs){
			debug("attempting to set prop %s to null", s);
			this.property_set(s, null);
		}
	}
	
	public void update(HashTable<string, Value?> data, HashSet<string> attributes)
	{
		debug("PlayerItem::update()");
		if(ensure_valid_updates(data, attributes) == false){
			debug("PlayerItem::Update -> The hashtable update does not contain what we were expecting - just leave it!");
			return;
		}
		foreach(string property in attributes){
			string[] input_keys = property.split("-");
			string search_key = input_keys[input_keys.length-1 : input_keys.length][0];
			debug("search key = %s", search_key);
			Value? v = data.lookup(search_key);
			
			if (v.holds (typeof (string))){
				string update = v.get_string().strip();
				debug("with value : %s", update);
				if(property.contains("arturl")){
					try{
						update = Filename.from_uri(update.strip());
					}
					catch(ConvertError e){
						warning("Problem converting URI %s to file path", update); 
					}
				}
				this.property_set(property, update);
			}			    
			else if (v.holds (typeof (int))){
				debug("with value : %i", v.get_int());
				this.property_set_int(property, v.get_int());
			}
			else if(v.holds (typeof (bool))){
				this.property_set_bool(property, v.get_boolean());
			}
		}
	}	

	private static bool ensure_valid_updates(HashTable<string, Value?> data, HashSet<string> attributes)
	{
		if(data == null){
			return false;
		}
		if(data.size() < attributes.size){
			warning("update hash was too small for the target");
			return false;
		}
		return true;		
	}

}

