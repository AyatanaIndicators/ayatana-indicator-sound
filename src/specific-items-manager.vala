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
	private PlayerController owner {get; set;}
  private string dbus_path;
  private Dbusmenu.Client client;
  private Gee.ArrayList<Dbusmenu.MenuitemProxy> proxy_items;
  
  public SpecificItemsManager (PlayerController controller, string path)
	{
    this.proxy_items = new ArrayList<Dbusmenu.MenuitemProxy>();
    this.owner = controller;
    this.dbus_path = path;
    this.client = new Dbusmenu.Client (this.owner.dbus_name, this.dbus_path);
    this.client.root_changed.connect (on_root_changed);
	}
  
  private void on_root_changed (GLib.Object newroot)
  {
    Dbusmenu.Menuitem root = this.client.get_root();
    root.child_added.connect (on_child_added);
    root.child_removed.connect (on_child_removed);

    // Fetch what children are there already.
    GLib.List<weak void*> children = root.get_children().copy();
    
    debug ("on_root_changed - size of children list : %i",
          (int)children.length());
    foreach (void* child in children) {
      unowned Dbusmenu.Menuitem item = (Dbusmenu.Menuitem)child;
      Dbusmenu.MenuitemProxy proxy = new Dbusmenu.MenuitemProxy(item);
      proxy_items.add (proxy);
      debug ("Proxy item of label = %s added to collection",
              item.property_get (MENUITEM_PROP_LABEL));
      this.owner.root_menu.child_add_position (proxy,
                                               this.owner.menu_offset + 3);
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
