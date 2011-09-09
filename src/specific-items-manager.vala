/*
Copyright 2011 Canonical Ltd.

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

public class SpecificItemsManager : GLib.Object
{
  public enum category{
    TRACK,
    PLAYER
  }

	private PlayerController owner {get; set;}
  private string dbus_path;
  private Dbusmenu.Client client;
  public Gee.ArrayList<Dbusmenu.MenuitemProxy> proxy_items {get; construct;}
  private int of_type;
   
  public SpecificItemsManager (PlayerController controller,
                               string path,
                               category which_type)
	{
    this.of_type = which_type;
    this.owner = controller;
    this.dbus_path = path;
    this.client = new Dbusmenu.Client (this.owner.dbus_name, this.dbus_path);
    this.client.root_changed.connect (on_root_changed);
	}
  construct{
    this.proxy_items = new ArrayList<Dbusmenu.MenuitemProxy>();    
  }
  
  private int figure_out_positioning()
  {
    int result = 0 ;
    if (this.of_type == category.TRACK){
      result = this.owner.menu_offset + this.owner.WIDGET_QUANTITY + this.proxy_items.size; 
    }
    else if (this.of_type == category.PLAYER){
      int pos = this.owner.menu_offset + this.owner.WIDGET_QUANTITY + this.owner.track_specific_count();
      //Surely the playlists item is there whether its visible or not ?
      //TODO test playlists and player specific item positioning.
      pos  += this.owner.use_playlists == true ? 1 : 0;
      result = pos;
    }
    debug ("!!!!! Menu pos of type %i is = %i", this.of_type, result);
    return result;
  }
  
  private void on_root_changed (GLib.Object? newroot)
  {
    if (newroot == null){
      debug ("root disappeared -remove proxyitems");
      foreach(var p in proxy_items){
        this.owner.root_menu.child_delete (p);  
      }      
      this.proxy_items.clear();
      debug ("array list size is now %i", this.proxy_items.size);
      //this.proxy_items = new ArrayList<Dbusmenu.MenuitemProxy>();      
      return;  
    }
    
    Dbusmenu.Menuitem? root = this.client.get_root();
    root.child_added.connect (on_child_added);
    root.child_removed.connect (on_child_removed);

    // Fetch what children are there already.
    GLib.List<weak void*> children = root.get_children().copy();
    
    foreach (void* child in children) {
      int pos = figure_out_positioning();
      unowned Dbusmenu.Menuitem item = (Dbusmenu.Menuitem)child;
      Dbusmenu.MenuitemProxy proxy = new Dbusmenu.MenuitemProxy(item);
      proxy_items.add (proxy);
      debug ("Proxy item of label = %s added to collection",
              item.property_get (MENUITEM_PROP_LABEL));
      this.owner.root_menu.child_add_position (proxy, pos);
    
    }
  }
  
  private void on_child_added (GLib.Object child, uint position)
  {
    debug ("On child added Specific root node");
  }

  private void on_child_removed (GLib.Object child)
  {
    debug ("On child removed Specific root node");
  }
  
}
