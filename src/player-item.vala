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
  public const int EMPTY = -1;

  public PlayerItem(string type)
  {   
    Object(item_type: type);
  }
  
  construct {
    this.property_set(MENUITEM_PROP_TYPE, item_type);
  }

  public void reset(HashSet<string> attrs){   
    foreach(string s in attrs){
      this.property_set_int(s, EMPTY);
    }
  }
  
  /**
   * update()
   * Base update method for playeritems, takes the attributes and the incoming updates
   * and attmepts to update the appropriate props on the object. 
   * Album art is handled separately to deal with remote and local file paths.
   */
  public void update(HashTable<string, Variant?> data, HashSet<string> attributes)
  {
    //debug("PlayerItem::update()");
    if(data == null){
      warning("PlayerItem::Update -> The hashtable was null - just leave it!");
      return;
    }
    
    foreach(string property in attributes){
      string[] input_keys = property.split("-");
      string search_key = input_keys[input_keys.length-1 : input_keys.length][0];
      //debug("search key = %s", search_key);
      Variant? v = data.lookup(search_key);
      
      if (v == null) continue;
      
      if (v.is_of_type ( VariantType.STRING )){
        string update = v.get_string().strip();
        //debug("with value : %s", update);
        if(property.contains("mpris:artUrl")){    
            // We know its a metadata instance because thats the only
            // object with the arturl prop            
            MetadataMenuitem metadata = this as MetadataMenuitem;
            metadata.fetch_art ( update, property );
            continue;                     
        }
        this.property_set(property, update);                      
      }         
      else if (v.is_of_type (VariantType.INT32 )){
        //debug("with value : %i", v.get_int32());
        this.property_set_int(property, v.get_int32());
      }
      else if (v.is_of_type (VariantType.INT64 )){
        //debug("with value : %i", (int)v.get_int64());
        this.property_set_int(property, (int)v.get_int64());
      }
      else if(v.is_of_type ( VariantType.BOOLEAN )){
        //debug("with value : %s", v.get_boolean().to_string());        
        this.property_set_bool(property, v.get_boolean());
      }
    }
  } 
  
  public bool populated(HashSet<string> attrs)
  {
    foreach(string prop in attrs){
      if(property_get_int(prop) != EMPTY){
        return true;
      }
    }
    return false;
  }

}

