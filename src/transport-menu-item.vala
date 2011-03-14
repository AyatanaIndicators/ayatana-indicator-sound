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
  private Transport.Action cached_action;
 
  private bool running {
    get{
      return this.owner.current_state == PlayerController.state.CONNECTED;
    } 
  }

  public TransportMenuitem(PlayerController parent)
  {
    Object(item_type: MENUITEM_TYPE, owner: parent);
  }
  construct{
    this.property_set_int(MENUITEM_PLAY_STATE, (int)Transport.State.PAUSED);
    this.cached_action = Transport.Action.NO_ACTION;
  }

  public void handle_cached_action()
  {
    if (this.cached_action != Transport.Action.NO_ACTION){ 
     debug ("TRYING TO FIRE OF A CACHED ACTION %i", (int)this.cached_action);
     Timeout.add_seconds (2, send_cached_action);
     //this.owner.mpris_bridge.transport_update(this.cached_action);        
    }
  }

  private bool send_cached_action()
  {
    this.owner.mpris_bridge.transport_update(this.cached_action);
    this.cached_action = Transport.Action.NO_ACTION;
    return false;
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
    debug ( "Handle event in transport menu item - is the player actually running %s", 
             this.running.to_string() );
    Variant v = input_value;
    if ( input_value.is_of_type (VariantType.VARIANT)){
      v = input_value.get_variant();
    }
    
    int32 input = v.get_int32();
    
    if (this.running == true){
      //debug("transport menu item -> handle_event with value %s", input.to_string());
      //debug("transport owner name = %s", this.owner.app_info.get_name());
      this.owner.mpris_bridge.transport_update((Transport.Action)input);
    }
    else{
      debug("transport cached action = %i", (Transport.Action)input);

      this.cached_action = (Transport.Action)input;
      this.owner.instantiate();
    }
  } 

  public static HashSet<string> attributes_format()
  {
    HashSet<string> attrs = new HashSet<string>();    
    attrs.add(MENUITEM_PLAY_STATE);
    return attrs;
  } 

}
