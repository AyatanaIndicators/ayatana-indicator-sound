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
using Transport;

public class TransportMenuitem : PlayerItem
{
  
  public TransportMenuitem(PlayerController parent)
  {
    Object(item_type: MENUITEM_TYPE, owner: parent);
    this.property_set_int(MENUITEM_PLAY_STATE, 1);
  }

  public void change_play_state(Transport.State update)
  {
    //debug("UPDATING THE TRANSPORT DBUSMENUITEM PLAY STATE WITH VALUE %i",
    //      (int)update);
    int temp = (int)update;
    this.property_set_int(MENUITEM_PLAY_STATE, temp); 
  }
  
  public override void handle_event(string name,
                                    Variant input_value,
                                    uint timestamp)
  {
    /*debug ( "Handle event in transport menu item - input variant is of type %s", 
             input_value.get_type_string() );*/
    Variant v = input_value;
    if ( input_value.is_of_type ( VariantType.VARIANT) ){
      v = input_value.get_variant();
    }
    
    int32 input = v.get_int32();
    //debug("transport menu item -> handle_event with value %s", input.to_string());
    //debug("transport owner name = %s", this.owner.app_info.get_name());
    this.owner.mpris_bridge.transport_update((Transport.Action)input);
  } 

  public static HashSet<string> attributes_format()
  {
    HashSet<string> attrs = new HashSet<string>();    
    attrs.add(MENUITEM_PLAY_STATE);
    return attrs;
  } 

}