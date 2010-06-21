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
	
	public void update(HashTable<string, Value?> data, HashMap<string, Type> attributes)
	{
		debug("PlayerItem::update()");
		foreach(var property in attributes){
			//debug("property value = %s", attributes.get(property.key).name());
			GLib.Object obj = GLib.Object.new(attributes.get(property.key));
			debug("bang line");
			Type t = obj.get_type();
			if(t.is_a(typeof(string)) == true){
				debug("obj is a string !!! we can tell -halla fucking lula!");
			}
			//string[] input_keys = property.key.split("-");
			//string search_key = input_keys[input_keys.length-1 : input_keys.length][0];
			//debug("key parsed from properties is %s", search_key);
			//debug("and the bloody type should be %s", property.value.name());
			//var input_value = data.lookup(search_key) as property.value.name();
			//foreach(var s in input_keys){
			//	debug("string = %s", s);
			//}
			//string[] st = input_keys[input_keys.length-1 : input_keys.length];
			//property.value as attributes.get(property.key)
			//this.property_set(property.key, );
		}
	}

	public void set_adaptor(MprisController adaptor)
	{
		this.mpris_adaptor = adaptor;		
	}

	public static HashMap<string, string> format_updates(HashTable<string,Value?> ht)
	{
		HashMap<string,string> results = new HashMap<string, string>();		
		//HashMap<string, Type> attrs = attributes_format();
		//results.set(MENUITEM_TEXT_TITLE, (string)data.lookup("title").strip());
    //results.set(MENUITEM_TEXT_ARTIST, (string)data.lookup("artist").strip());
    //results.set(MENUITEM_TEXT_ALBUM, (string)data.lookup("album").strip(), typeof(string));
    //results.set(MENUITEM_TEXT_ARTURL, sanitize_image_path((string)data.lookup("arturl").strip()), typeof(string));
		return results;
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

