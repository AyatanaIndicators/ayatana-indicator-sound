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

public class PlayerController : GLib.Object
{
  public const int WIDGET_QUANTITY = 4;

  public enum widget_order{
    SEPARATOR,
    METADATA,
    TRANSPORT,
    PLAYLISTS
  }

  public enum state{
    OFFLINE,
    INSTANTIATING,
    READY,
    CONNECTED,
    DISCONNECTED
  }
  
  public int current_state = state.OFFLINE;
    
  public Dbusmenu.Menuitem root_menu;
  public string dbus_name { get; set;}
  public ArrayList<PlayerItem> custom_items;
  public Mpris2Controller mpris_bridge;
  public AppInfo? app_info { get; set;}
  public int menu_offset { get; set;}
  public string icon_name { get; set; }
  public bool? use_playlists;
  private SpecificItemsManager track_specific_mgr;
  private SpecificItemsManager player_specific_mgr;
  
  public PlayerController(Dbusmenu.Menuitem root,
                          GLib.AppInfo app,
                          string? dbus_name,
                          string icon_name,
                          int offset,
                          bool? use_playlists,
                          state initial_state)
  {
    this.use_playlists = use_playlists;
    this.root_menu = root;
    this.app_info = app;
    this.dbus_name = dbus_name;
    this.icon_name = icon_name;
    this.custom_items = new ArrayList<PlayerItem>();
    this.current_state = initial_state;
    this.menu_offset = offset;
    this.construct_widgets();
    this.establish_mpris_connection();
    this.update_layout();
    debug ("New player controller  for %s with icon name %s", this.app_info.get_name(), this.icon_name);
  }

  public void update_state(state new_state)
  {
    debug("update_state - player controller %s : new state %i", this.app_info.get_name(),
                                                                new_state);
    this.current_state = new_state;
    this.update_layout();
  }
  
  public void activate( string dbus_name )
  {
    this.dbus_name = dbus_name;
    this.establish_mpris_connection();
  }

  /*
   instantiate()
   The user should be able to start the app from the transport bar when in an offline state
   There is a need to wait before the application is on DBus before attempting to access its mpris address
   Hence only when the it has registered with us via libindicate do we attempt to kick off mpris communication
   */
  public void instantiate()
  {
    debug("instantiate in player controller for %s", this.app_info.get_name() );
    try{
      this.app_info.launch(null, null);
      this.update_state(state.INSTANTIATING);
    }
    catch(GLib.Error error){
      warning("Failed to launch app %s with error message: %s", this.app_info.get_name(),
                                                                error.message );
    }
  }

  public void enable_track_specific_items (string object_path)
  { 
    if (this.track_specific_mgr == null){
      track_specific_mgr = new SpecificItemsManager (this,
                                                     object_path,
                                                     SpecificItemsManager.category.TRACK);
    }
  }

  public void enable_player_specific_items (string object_path)
  { 
    if (this.player_specific_mgr == null){
      player_specific_mgr = new SpecificItemsManager (this,
                                                      object_path,
                                                      SpecificItemsManager.category.PLAYER);
    }
  }
  
  public int track_specific_count ()
  {
    if (this.track_specific_mgr == null) {
      return 0;    
    }
    return this.track_specific_mgr.proxy_items.size;
  }  
    
  private void establish_mpris_connection()
  {   
    if(this.current_state != state.READY || this.dbus_name == null ){
      debug("establish_mpris_connection - Not ready to connect");
      return;
    }
    debug ( " establish mpris connection - use playlists value = %s ",
            this.use_playlists.to_string() );
    this.mpris_bridge = new Mpris2Controller (this);
    this.determine_state ();
  }
  
  public void remove_from_menu()
  {
    foreach(PlayerItem item in this.custom_items){
      this.root_menu.child_delete(item);
    }
    if (this.use_playlists == true){
      PlaylistsMenuitem playlists_menuitem = this.custom_items[widget_order.PLAYLISTS] as PlaylistsMenuitem;
      this.root_menu.child_delete (playlists_menuitem.root_item);
    }
  }

  public void hibernate()
  {
    update_state(PlayerController.state.OFFLINE);
    TransportMenuitem transport = this.custom_items[widget_order.TRANSPORT] as TransportMenuitem;
    transport.change_play_state (Transport.State.PAUSED);
    this.custom_items[widget_order.METADATA].reset(MetadataMenuitem.relevant_attributes_for_ui());
    MetadataMenuitem md = this.custom_items[widget_order.METADATA] as MetadataMenuitem;
    md.toggle_active_triangle (false);
    this.mpris_bridge = null;
  }

  public void update_layout()
  {    
    PlaylistsMenuitem playlists_menuitem = this.custom_items[widget_order.PLAYLISTS] as PlaylistsMenuitem;
    MetadataMenuitem metadata_menuitem = this.custom_items[widget_order.METADATA] as MetadataMenuitem;
    if(this.current_state != state.CONNECTED){
      metadata_menuitem.should_collapse (true);
      playlists_menuitem.root_item.property_set_bool (MENUITEM_PROP_VISIBLE,
                                                      false);
      this.custom_items[widget_order.TRANSPORT].property_set_bool (MENUITEM_PROP_VISIBLE,
                                                                   this.app_info.get_id() == "rhythmbox.desktop");         
      return; 
    }
    metadata_menuitem.should_collapse (!this.custom_items[widget_order.METADATA].populated (MetadataMenuitem.relevant_attributes_for_ui()) );
    if (this.app_info.get_id() == "rhythmbox.desktop"){
      TransportMenuitem transport = this.custom_items[widget_order.TRANSPORT] as TransportMenuitem;
      transport.handle_cached_action();
    }
    else{
      this.custom_items[widget_order.TRANSPORT].property_set_bool (MENUITEM_PROP_VISIBLE,
                                                                   true);         
    }
    playlists_menuitem.root_item.property_set_bool ( MENUITEM_PROP_VISIBLE,
                                                     this.use_playlists );
  }
    
  private void construct_widgets()
  {
    // Separator item
    this.custom_items.add(new PlayerItem(CLIENT_TYPES_SEPARATOR));

    // Metadata item
    MetadataMenuitem metadata_item = new MetadataMenuitem(this);
    this.custom_items.add(metadata_item);

    // Transport item
    TransportMenuitem transport_item = new TransportMenuitem(this);
    this.custom_items.add(transport_item);
    
    // Playlist item
    PlaylistsMenuitem playlist_menuitem = new PlaylistsMenuitem(this);
    this.custom_items.add(playlist_menuitem);
    
    foreach(PlayerItem item in this.custom_items){
      if (this.custom_items.index_of(item) == WIDGET_QUANTITY-1) {
        PlaylistsMenuitem playlists_menuitem = item as PlaylistsMenuitem;
        root_menu.child_add_position(playlists_menuitem.root_item, this.menu_offset + this.custom_items.index_of(item));
      }
      else{
        root_menu.child_add_position (item,
                                      this.menu_offset + this.custom_items.index_of(item));
      }
    }
  }
     
  private void determine_state()
  {
    if(this.mpris_bridge.connected() == true){
      this.update_state(state.CONNECTED);
      MetadataMenuitem md = this.custom_items[widget_order.METADATA] as MetadataMenuitem;
      md.toggle_active_triangle(true);
      this.mpris_bridge.initial_update();
    }
    else{
      this.update_state(state.DISCONNECTED);
    }
  }
}
