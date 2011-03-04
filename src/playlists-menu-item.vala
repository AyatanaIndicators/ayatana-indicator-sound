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
using DbusmenuPlaylists;
using DbusmenuPlaylist;
using Gee;

public class PlaylistsMenuitem : PlayerItem
{
  private HashMap<int, Dbusmenu.Menuitem> current_playlists;    
  public Menuitem root_item;
  
  public PlaylistsMenuitem ( PlayerController parent )
  {
    Object ( item_type: MENUITEM_TYPE, owner: parent );
  }
  construct{
    this.current_playlists = new HashMap<int, Dbusmenu.Menuitem>();
    this.root_item = new Menuitem();

    this.root_item.property_set ( MENUITEM_PROP_LABEL, "Choose Playlist" );
    this.root_item.property_set ( MENUITEM_PATH, "" );
  }

  public new void update (PlaylistDetails[] playlists)
  {
    foreach ( PlaylistDetails detail in playlists ){
      
      if (this.already_observed(detail)) continue;
      
      Dbusmenu.Menuitem menuitem = new Menuitem();
      menuitem.property_set (MENUITEM_PROP_LABEL, detail.name);

      var icon_file = File.new_for_path (detail.icon_path);
      if (icon_file.query_exists () == true) {
        debug ("icon name = %s", icon_file.get_basename ());
        menuitem.property_set (MENUITEM_PROP_ICON_NAME,
                               icon_file.get_basename () );
      }
      menuitem.property_set (MENUITEM_PROP_ICON_NAME, detail.icon_path);
      menuitem.property_set (MENUITEM_PATH, (string)detail.path);
      menuitem.property_set_bool (MENUITEM_PROP_VISIBLE, true);
      menuitem.property_set_bool (MENUITEM_PROP_ENABLED, true);
      
      menuitem.item_activated.connect(() => {
        submenu_item_activated (menuitem.id );
        }
      );
      this.current_playlists.set( menuitem.id, menuitem ); 
      this.root_item.child_append( menuitem );
    }
    // Finally remove any that might have been deleted
    foreach (Dbusmenu.Menuitem item in this.current_playlists.values) {
      bool within = false;
      foreach (PlaylistDetails detail in playlists){
        if (detail.path == item.property_get (MENUITEM_PATH)) {
          within = true;
          break;
        }
      }
      if (within == false){
        if (this.root_item.property_get (MENUITEM_PATH) == item.property_get (MENUITEM_PATH)){
          this.root_item.property_set (MENUITEM_PROP_LABEL, "Choose Playlist");        
        }
        this.root_item.child_delete (item);   
      }
    }
  }

  public void update_individual_playlist (PlaylistDetails new_detail)
  {
    foreach ( Dbusmenu.Menuitem item in this.current_playlists.values ){
      if (new_detail.path == item.property_get (MENUITEM_PATH)){
        item.property_set (MENUITEM_PROP_LABEL, new_detail.name);
        item.property_set (MENUITEM_PROP_ICON_NAME, new_detail.icon_path);      
      }
    }
    // If its active make sure the name is updated on the root item.
    if (this.root_item.property_get (MENUITEM_PATH) == new_detail.path) {
      this.root_item.property_set (MENUITEM_PROP_LABEL, new_detail.name);          
    }
  }
                                                  
  private bool already_observed (PlaylistDetails new_detail)
  {
    foreach ( Dbusmenu.Menuitem item in this.current_playlists.values ){
      var path = item.property_get (MENUITEM_PATH);
      if (new_detail.path == path) return true;
    }
    return false;
  }

  public void active_playlist_update (PlaylistDetails detail)
  {
    var update = detail.name; 
    if ( update == "" ) update = "Choose Playlist";
    this.root_item.property_set (MENUITEM_PROP_LABEL, update);  
    this.root_item.property_set (MENUITEM_PATH, detail.path);  
  }
  
  private void submenu_item_activated (int menu_item_id)
  {
    if (!this.current_playlists.has_key(menu_item_id)) {
      warning( "item %i was activated but we don't have a corresponding playlist",
               menu_item_id );
      return;
    }
    this.owner.mpris_bridge.activate_playlist ( (GLib.ObjectPath)this.current_playlists[menu_item_id].property_get (MENUITEM_PATH) );
  }
  
  public static HashSet<string> attributes_format()
  {
    HashSet<string> attrs = new HashSet<string>();    
    attrs.add(MENUITEM_TITLE);
    attrs.add(MENUITEM_PLAYLISTS);
    return attrs;
  } 
  
}
