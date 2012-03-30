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
using Transport;

public class Mpris2Controller : GLib.Object
{
  public const int MAX_PLAYLIST_COUNT = 100;

  public MprisRoot mpris2_root {get; construct;}    
  public MprisPlayer player {get; construct;}
  public MprisPlaylists playlists {get; construct;}
  public FreeDesktopProperties properties_interface {get; construct;}
  public PlayerController owner {get; construct;}

  public Mpris2Controller(PlayerController ctrl)
  {
    GLib.Object(owner: ctrl); 
  }

  construct{
    try {
      this.mpris2_root = Bus.get_proxy_sync ( BusType.SESSION,
                                              this.owner.dbus_name,
                                              "/org/mpris/MediaPlayer2" );
      this.player = Bus.get_proxy_sync ( BusType.SESSION,
                                         this.owner.dbus_name,
                                         "/org/mpris/MediaPlayer2" );
      this.properties_interface = Bus.get_proxy_sync ( BusType.SESSION,
                                                       "org.freedesktop.Properties.PropertiesChanged",
                                                       "/org/mpris/MediaPlayer2" );
      this.properties_interface.PropertiesChanged.connect ( property_changed_cb );
      if ( this.owner.use_playlists == true ){
        this.playlists = Bus.get_proxy_sync ( BusType.SESSION,
                                              this.owner.dbus_name,
                                              "/org/mpris/MediaPlayer2" );
        this.playlists.PlaylistChanged.connect (on_playlistdetails_changed);
      }
    }
    catch (IOError e) {
      critical("Problems connecting to the session bus - %s", e.message);
    }
  }
  /*
   * property_changed_cb
   * Called when a property changed signal is emitted from any of mpris
   * objects on the bus.
   * Note that the signal will be received by each instance for each player
   * and at that moment there is no way to know what player that signal 
   * came from therefore it is necessary to query each relevant property
   * to update the respective dbusmenuitem property inorder to keep the UI in sync
   * Please also note due to some race condition in the depths of gdbus
   * a timeout is needed between receiving the prop update and query the respective property.
   * This can be seen at various points below.
   */
  public void property_changed_cb ( string interface_source,
                                    HashTable<string, Variant?> changed_properties,
                                    string[] invalid )
  {
    if ( changed_properties == null ||
        interface_source.has_prefix ( MPRIS_PREFIX ) == false ){
      warning("Property-changed hash is null or this is an interface that doesn't concern us");
      return;
    }
    Variant? play_v = changed_properties.lookup("PlaybackStatus");
    if(play_v != null){
      string state = this.player.PlaybackStatus;
      Timeout.add ( 200, ensure_correct_playback_status );
      Transport.State p = (Transport.State)this.determine_play_state(state);
      (this.owner.custom_items[PlayerController.widget_order.TRANSPORT] as TransportMenuitem).change_play_state(p);
    }
    Variant? meta_v = changed_properties.lookup("Metadata");
    if(meta_v != null)
    {
      Timeout.add ( 200, ensure_correct_metadata );
    }
    Variant? playlist_v = changed_properties.lookup("ActivePlaylist");
    if ( playlist_v != null && this.owner.use_playlists == true ){
      Timeout.add (500, this.fetch_active_playlist);
    }
    Variant? playlist_count_v = changed_properties.lookup("PlaylistCount");
    if ( playlist_count_v != null && this.owner.use_playlists == true ){
      this.fetch_playlists.begin();
      this.fetch_active_playlist();
    }
    Variant? playlist_orderings_v = changed_properties.lookup("Orderings");
    if ( playlist_orderings_v != null && this.owner.use_playlists == true ){
      this.fetch_playlists.begin();
      this.fetch_active_playlist();
    }
    Variant? identity_v = changed_properties.lookup("Identity");
    if (identity_v != null){
      MetadataMenuitem md = this.owner.custom_items[PlayerController.widget_order.METADATA] as MetadataMenuitem;      
      md.alter_label (this.mpris2_root.Identity);
    }
  }
         
  private bool ensure_correct_metadata ()
  {
    GLib.HashTable<string, Variant?> changed_updates = clean_metadata();      
    PlayerItem metadata = this.owner.custom_items[PlayerController.widget_order.METADATA];
    metadata.reset (MetadataMenuitem.relevant_attributes_for_ui());
    metadata.update ( changed_updates, 
                      MetadataMenuitem.relevant_attributes_for_ui());
    MetadataMenuitem md = this.owner.custom_items[PlayerController.widget_order.METADATA] as MetadataMenuitem;      
    bool collapsing = !metadata.populated(MetadataMenuitem.relevant_attributes_for_ui());
    md.should_collapse(collapsing);
    
    return false;
  }
  
  private bool ensure_correct_playback_status()
  {
    Transport.State p = (Transport.State)this.determine_play_state(this.player.PlaybackStatus);
    (this.owner.custom_items[PlayerController.widget_order.TRANSPORT] as TransportMenuitem).change_play_state(p);
    return false;
  }
  
  private GLib.HashTable<string, Variant?>? clean_metadata()
  { 
    GLib.HashTable<string, Variant?> changed_updates = this.player.Metadata; 
    
    Variant? artist_v = this.player.Metadata.lookup("xesam:artist");
    if(artist_v != null){
      string display_artists;
      // Accomodate Spotify (should return 'as' and not 's') 
      if(artist_v.get_type_string() == "s"){
        display_artists = artist_v.get_string();
      }
      else{
        string[] artists = artist_v.dup_strv();
        display_artists = string.joinv(", ", artists);
      }
      changed_updates.replace("xesam:artist", display_artists);
    }
    return changed_updates;
  }
  
  private Transport.State determine_play_state(string? status){
    if(status != null && status == "Playing"){
      return Transport.State.PLAYING;
    }
    return Transport.State.PAUSED;
  }

  public void initial_update()
  {
    Transport.State update;
    
    if(this.player.PlaybackStatus == null){
      update = Transport.State.PAUSED;
    }
    else{
      update = determine_play_state (this.player.PlaybackStatus);
    }
    if (this.mpris2_root.Identity != null){
      MetadataMenuitem md = this.owner.custom_items[PlayerController.widget_order.METADATA] as MetadataMenuitem;      
      md.alter_label (this.mpris2_root.Identity);
    }
    (this.owner.custom_items[PlayerController.widget_order.TRANSPORT] as TransportMenuitem).change_play_state (update);
    GLib.HashTable<string, Value?>? cleaned_metadata = this.clean_metadata();
    this.owner.custom_items[PlayerController.widget_order.METADATA].update (cleaned_metadata,
                                                                            MetadataMenuitem.attributes_format());

    if ( this.owner.use_playlists == true ){
      this.fetch_playlists.begin();
      this.fetch_active_playlist();
    }
  }

  public void transport_update(Transport.Action command)
  {
    if(command == Transport.Action.PLAY_PAUSE){
      this.player.PlayPause.begin();              
    }
    else if(command == Transport.Action.PREVIOUS){
      this.player.Previous.begin();
    }
    else if(command == Transport.Action.NEXT){
      this.player.Next.begin();
    }
    else if(command == Transport.Action.REWIND){
     this.player.Seek.begin(-500000);
    }
    else if(command == Transport.Action.FORWIND){
     this.player.Seek.begin(400000);
    }
  }

  public bool connected()
  {
    return (this.player != null && this.mpris2_root != null);
  }

  public void expose()
  {
    if(this.connected() == true){
      this.mpris2_root.Raise.begin();
    }
  }

  private void on_playlistdetails_changed (PlaylistDetails details)
  {
    PlaylistsMenuitem playlists_item = this.owner.custom_items[PlayerController.widget_order.PLAYLISTS] as PlaylistsMenuitem;
    playlists_item.update_individual_playlist (details);    
  }

  public async void fetch_playlists()
  {
    PlaylistDetails[] current_playlists = null;
    
    try{   
      current_playlists =  yield this.playlists.GetPlaylists (0,
                                                              MAX_PLAYLIST_COUNT,
                                                              "Alphabetical",
                                                              false);
    }
    catch (IOError e){
      return;
    }
    
    if( current_playlists != null ){
      PlaylistsMenuitem playlists_item = this.owner.custom_items[PlayerController.widget_order.PLAYLISTS] as PlaylistsMenuitem;
      playlists_item.update(current_playlists);
    }
    else{
      warning(" Playlists are on but %s is returning no current_playlists ?",
              this.owner.app_info.get_name());
      this.owner.use_playlists = false;
    }
  }

  private bool validate_playlists_details()
  {
    if (this.playlists.ActivePlaylist == null){
      return false;
    }
    if (this.playlists.ActivePlaylist.valid == false){
      return false;
    }    
    if (this.playlists.ActivePlaylist.details == null){
      return false;
    }
    if (this.playlists.ActivePlaylist.details.path == null ||
        this.playlists.ActivePlaylist.details.name == null){
      return false;      
    } 
    return true;
  }

  private bool fetch_active_playlist()
  {    
    if (this.validate_playlists_details() == false){
      return false;
    }    
    PlaylistsMenuitem playlists_item = this.owner.custom_items[PlayerController.widget_order.PLAYLISTS] as PlaylistsMenuitem;
    playlists_item.active_playlist_update ( this.playlists.ActivePlaylist.details );
    return false;
  }

  public void activate_playlist (ObjectPath path)
  {
    try{
      this.playlists.ActivatePlaylist.begin(path);
    }
    catch(IOError e){
      warning ("Could not activate playlist %s because %s", (string)path, e.message);
    }
  }
}
