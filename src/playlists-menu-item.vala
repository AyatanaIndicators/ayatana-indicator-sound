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
using Gee;

public class PlaylistsMenuitem : PlayerItem
{
  private HashMap<int, PlaylistDetails?> current_playlists;    
  public Menuitem root_item;
  public PlaylistsMenuitem ( PlayerController parent )
  {
    Object ( item_type: MENUITEM_TYPE, owner: parent );
  }
  construct{
    this.current_playlists = new HashMap<int, PlaylistDetails?>();
    this.root_item = new Menuitem();
    this.root_item.property_set ( MENUITEM_PROP_LABEL, "Choose Playlist" );
  }

  public new void update (PlaylistDetails[] playlists)
  {
    foreach ( PlaylistDetails detail in playlists ){
      Dbusmenu.Menuitem menuitem = new Menuitem();
      menuitem.property_set (MENUITEM_PROP_LABEL, detail.name);
      menuitem.property_set (MENUITEM_PROP_ICON_NAME, "source-smart-playlist");
      menuitem.property_set_bool (MENUITEM_PROP_VISIBLE, true);
      menuitem.property_set_bool (MENUITEM_PROP_ENABLED, true);
      this.current_playlists.set( menuitem.id, detail );
      menuitem.item_activated.connect(() => {
        submenu_item_activated (menuitem.id );});
      this.root_item.child_append( menuitem );
    }
  }

  public void update_active_playlist(PlaylistDetails detail)
  {
    this.root_item.property_set ( MENUITEM_PROP_LABEL, detail.name );    
  }
  
  private void submenu_item_activated (int menu_item_id)
  {
    if(!this.current_playlists.has_key(menu_item_id)){
      warning( "item %i was activated but we don't have a corresponding playlist",
               menu_item_id );
      return;
    }
    this.owner.mpris_bridge.activate_playlist ( this.current_playlists[menu_item_id].path );
  }
  
  public static HashSet<string> attributes_format()
  {
    HashSet<string> attrs = new HashSet<string>();    
    attrs.add(MENUITEM_TITLE);
    attrs.add(MENUITEM_PLAYLISTS);
    return attrs;
  } 
  
}
