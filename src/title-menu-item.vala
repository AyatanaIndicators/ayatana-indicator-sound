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
  }

  construct
  {  
    this.property_set(MENUITEM_NAME, this.owner.app_info.get_name());    
    this.property_set(MENUITEM_ICON, this.owner.icon_name);    
    this.property_set_bool(MENUITEM_RUNNING, false);        
  }

  public override void handle_event(string name, Variant input_value, uint timestamp)
  {   
    if(this.owner.current_state == PlayerController.state.OFFLINE)
    {
      this.owner.instantiate();
    }
    else if(this.owner.current_state == PlayerController.state.CONNECTED){
      this.owner.mpris_bridge.expose();
    }   
  }

  public void alter_label (string new_title)
  {
    if (new_title != null) return;
    this.property_set(MENUITEM_NAME, new_title);        
  }

  public void toggle_active_triangle(bool update)
  {
    this.property_set_bool(MENUITEM_RUNNING, update);       
  }

  public static HashSet<string> attributes_format()
  {
    HashSet<string> attrs = new HashSet<string>();    
    attrs.add(MENUITEM_NAME);
    attrs.add(MENUITEM_RUNNING);
    attrs.add(MENUITEM_ICON);
    return attrs;
  } 
}