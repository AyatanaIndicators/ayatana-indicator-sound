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

/*
 This class will entirely replace mpris-controller.vala hence why there is no
 point in trying to get encorporate both into the same object model. 
 */
public class Mpris2Controller : GLib.Object
{
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
      warning("Problems connecting to the session bus - %s", e.message);
    }
  }

  public void property_changed_cb ( string interface_source,
                                    HashTable<string, Variant?> changed_properties,
                                    string[] invalid )
  {
    //debug("properties-changed for interface %s and owner %s", interface_source, this.owner.dbus_name);
    if ( changed_properties == null ||
        interface_source.has_prefix ( MPRIS_PREFIX ) == false ){
      warning("Property-changed hash is null or this is an interface that doesn't concern us");
      return;
    }
    Variant? play_v = changed_properties.lookup("PlaybackStatus");
    if(play_v != null){
      // Race condition sometimes appears with the playback status 
      // 200ms timeout ensures we have the correct playback status at all times.
      string state = this.player.PlaybackStatus;
      //debug("in the property update and the playback status = %s and update = %s", state, (string)play_v);
      Timeout.add ( 200, ensure_correct_playback_status );
      Transport.State p = (Transport.State)this.determine_play_state(state);
      (this.owner.custom_items[PlayerController.widget_order.TRANSPORT] as TransportMenuitem).change_play_state(p);
    }
    Variant? meta_v = changed_properties.lookup("Metadata");
    if(meta_v != null){
      GLib.HashTable<string, Variant?> changed_updates = clean_metadata();
      PlayerItem metadata = this.owner.custom_items[PlayerController.widget_order.METADATA];
      metadata.reset ( MetadataMenuitem.attributes_format());
      metadata.update ( changed_updates, 
                        MetadataMenuitem.attributes_format());
      metadata.property_set_bool ( MENUITEM_PROP_VISIBLE,
                                   metadata.populated(MetadataMenuitem.attributes_format()));
    }
    Variant? playlist_v = changed_properties.lookup("ActivePlaylist");
    if ( playlist_v != null && this.owner.use_playlists == true ){
      // Once again A GDBus race condition, the property_changed signal is sent
      // before the value is set on the respective property.
      Timeout.add (300, this.fetch_active_playlist);
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
      TitleMenuitem title = this.owner.custom_items[PlayerController.widget_order.TITLE] as TitleMenuitem;
      title.alter_label (this.mpris2_root.Identity);
    }
  }
                                      
  private bool ensure_correct_playback_status(){
    //debug("TEST playback status = %s", this.player.PlaybackStatus);
    Transport.State p = (Transport.State)this.determine_play_state(this.player.PlaybackStatus);
    (this.owner.custom_items[PlayerController.widget_order.TRANSPORT] as TransportMenuitem).change_play_state(p);
    return false;
  }
  
  private GLib.HashTable<string, Variant?>? clean_metadata()
  { 
    GLib.HashTable<string, Variant?> changed_updates = this.player.Metadata; 
    
    Variant? artist_v = this.player.Metadata.lookup("xesam:artist");
    if(artist_v != null){
      Variant? v_artists = this.player.Metadata.lookup("xesam:artist");
      //debug("artists is of type %s", v_artists.get_type_string ());
      string display_artists;
      if(v_artists.get_type_string() == "s"){
        //debug("SPOTIFY is that you ?");
        display_artists = v_artists.get_string();
      }
      else{
       string[] artists = v_artists.dup_strv();
        display_artists = string.joinv(", ", artists);
      }
      changed_updates.replace("xesam:artist", display_artists);
      //debug("artist : %s", (string)changed_updates.lookup("xesam:artist"));
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
      TitleMenuitem title = this.owner.custom_items[PlayerController.widget_order.TITLE] as TitleMenuitem;
      title.alter_label (this.mpris2_root.Identity);
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
    //debug("transport_event input = %i", (int)command);
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
     //debug("transport_event rewind = %i", (int)command);
     this.player.Seek.begin(-500000);
    }
    else if(command == Transport.Action.FORWIND){
     //debug("transport_event input = %i", (int)command);
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
                                                              10,
                                                              "Alphabetical",
                                                              false);
    }
    catch (IOError e){
      //debug("Could not fetch playlists because %s", e.message);
      return;
    }
    
    if( current_playlists != null ){
      //debug( "Size of the playlist array = %i", current_playlists.length );
      PlaylistsMenuitem playlists_item = this.owner.custom_items[PlayerController.widget_order.PLAYLISTS] as PlaylistsMenuitem;
      playlists_item.update(current_playlists);
    }
    else{
      warning(" Playlists are on but its returning no current_playlists" );
      this.owner.use_playlists = false;
    }
  }

  private bool fetch_active_playlist()
  {    
    if (this.playlists.ActivePlaylist.valid == false){
      //debug(" We don't have an active playlist");
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
      //debug("Could not activate playlist %s because %s", (string)path, e.message);
    }
  }
}
