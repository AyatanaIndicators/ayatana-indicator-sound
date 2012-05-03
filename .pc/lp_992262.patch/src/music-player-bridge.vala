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
using GLib;

public class MusicPlayerBridge : GLib.Object
{
  const int DEVICE_ITEMS_COUNT = 3;

  private SettingsManager settings_manager;
  private Dbusmenu.Menuitem root_menu;
  private HashMap<string, PlayerController> registered_clients;
  private HashMap<string, string> file_monitors;
  private Mpris2Watcher watcher;

  public MusicPlayerBridge()
  {
  }
  
  construct{
    this.registered_clients = new HashMap<string, PlayerController> ();
    this.file_monitors = new HashMap<string, string> ();
    this.settings_manager = new SettingsManager();
    this.settings_manager.blacklist_updates.connect ( this.on_blacklist_update );
  }
  
  private void on_blacklist_update ( string[] blacklist )
  {
    debug("some blacklist update");

    foreach(var s in blacklist){
      string key = this.determine_key (s);
      if (this.registered_clients.has_key (key)){
        debug ("Apparently %s is now blacklisted - remove thy self", key);
        this.registered_clients[key].remove_from_menu();
        this.registered_clients.unset (key);
      }
    }
    // double check present players to ensure dynamic removal/addition 
    this.watcher.check_for_active_clients.begin();
  }

  private void try_to_add_inactive_familiar_clients()
  {
    foreach ( string desktop in this.settings_manager.fetch_interested()){
      debug ( "interested client found : %s", desktop );
      AppInfo? app_info = create_app_info ( desktop.concat( ".desktop" ) );
      if ( app_info == null ){
        warning ( "Could not create app_info for path %s \n Getting out of here ",
                   desktop );
        continue;
      }
      var mpris_key = determine_key ( desktop );
      PlayerController ctrl = new PlayerController ( this.root_menu, 
                                                     app_info,
                                                     null,
                                                     this.fetch_icon_name(desktop),
                                                     calculate_menu_position(),
                                                     null,
                                                     PlayerController.state.OFFLINE );
      this.registered_clients.set(mpris_key, ctrl);  
      this.establish_file_monitoring (app_info, mpris_key);
    }
  }
  
  private void establish_file_monitoring (AppInfo info, string mpris_key){
      DesktopAppInfo desktop_info = info as DesktopAppInfo;
      var file_path = desktop_info.get_filename ();
      File f = File.new_for_path (file_path);
      try {
        FileMonitor monitor = f.monitor (FileMonitorFlags.SEND_MOVED, null);
        unowned FileMonitor weak_monitor = monitor;
        monitor.changed.connect ((desktop_file, other_file, event_type) => {
          this.relevant_desktop_file_changed (desktop_file, other_file, event_type, weak_monitor);
        });
        monitor.ref(); // will be unref()ed by relevant_desktop_file_changed()
        GLib.debug ("monitoring file '%s'", file_path);
        this.file_monitors.set (file_path, mpris_key);
      }
      catch (Error e){
        warning ("Unable to create a file monitor for %s", info.get_name());
        return;
      }
  }
  
  private void relevant_desktop_file_changed (File desktop_file,
                                              File? other_file,
                                              FileMonitorEvent event_type,
                                              FileMonitor monitor)
  {
    if (event_type != FileMonitorEvent.DELETED)
      return;
      
    string? path = desktop_file.get_path ();
    if (path == null){
      warning ("relevant_desktop_file_changed is returning a file with no path !");
      return;
    }
    if (!this.file_monitors.has_key (path)){
      warning ("relevant_desktop_file_changed is returning a file which we know nothing about - %s",
                path);
      return;
    }

    var mpris_key = this.file_monitors[path];
    GLib.debug ("file \"%s\" was removed; stopping monitoring \"%s\"", path, mpris_key);
    this.registered_clients[mpris_key].remove_from_menu();
    this.settings_manager.remove_interested (mpris_key);
    this.registered_clients.unset (mpris_key);
    monitor.cancel ();
    monitor.unref();
  }                                              

  private int calculate_menu_position()
  {
    if(this.registered_clients.size == 0){
      return DEVICE_ITEMS_COUNT;
    }
    else{
      return (DEVICE_ITEMS_COUNT + (this.registered_clients.size * PlayerController.WIDGET_QUANTITY));
    }
  }

  public void client_has_become_available ( string desktop,
                                            string dbus_name,
                                            bool use_playlists )
  {
    if (desktop == null || desktop == ""){
      warning("Client %s attempting to register without desktop entry being set on the mpris root",
               dbus_name);
      return;
    }
    if (desktop in this.settings_manager.fetch_blacklist()) {
      debug ("Client %s attempting to register but I'm afraid it is blacklisted",
             desktop);
      return;
    }
    
    debug ( "client_has_become_available %s", desktop );
    AppInfo? app_info = create_app_info ( desktop.concat( ".desktop" ) );
    if ( app_info == null ){
      warning ( "Could not create app_info for path %s \n Getting out of here ",
                 desktop );
      return;
    }
    
    var mpris_key = determine_key ( desktop );
    // Are we sure clients will appear like this with the new registration method in place. 
    if ( this.registered_clients.has_key (mpris_key) == false ){
      debug("New client has registered that we have not seen before: %s", dbus_name );
      PlayerController ctrl = new PlayerController ( this.root_menu,
                                                     app_info,
                                                     dbus_name,
                                                     this.fetch_icon_name(desktop),                                                    
                                                     this.calculate_menu_position(),
                                                     use_playlists,
                                                     PlayerController.state.READY );
      this.registered_clients.set ( mpris_key, ctrl );
      debug ( "Have not seen this %s before, new controller created.", desktop );        
      this.settings_manager.add_interested ( desktop );
      this.establish_file_monitoring (app_info, mpris_key);      
      debug ( "application added to the interested list" );
    }
    else{
      this.registered_clients[mpris_key].use_playlists = use_playlists;
      this.registered_clients[mpris_key].update_state ( PlayerController.state.READY );
      this.registered_clients[mpris_key].activate ( dbus_name );
      debug("Application has already registered - awaken the hibernation: %s with playlists %s \n", dbus_name, use_playlists.to_string() );
    }
  }
  
  public void client_has_vanished ( string mpris_root_interface )
  {
    debug("MusicPlayerBridge -> client with dbus interface %s has vanished",
           mpris_root_interface );
    if (root_menu != null){
      debug("attempt to remove %s", mpris_root_interface);
      var mpris_key = determine_key ( mpris_root_interface );
      if ( mpris_key != null && this.registered_clients.has_key(mpris_key)){
        registered_clients[mpris_key].hibernate();
        debug("Successively offlined client %s", mpris_key);       
      }
    }
  }
 
  public void set_root_menu_item ( Dbusmenu.Menuitem menu )
  {
    this.root_menu = menu;
    this.try_to_add_inactive_familiar_clients();
    this.watcher = new Mpris2Watcher ();
    this.watcher.client_appeared.connect (this.client_has_become_available);
    this.watcher.client_disappeared.connect (this.client_has_vanished);
  }

  public void enable_player_specific_items_for_client (string object_path,
                                                       string desktop_id)
  {
    var mpris_key = determine_key ( desktop_id );
    if (this.registered_clients.has_key (mpris_key) == false){
      warning ("we don't have a client with desktop id %s registered", desktop_id);
      return;
    }
    this.registered_clients[mpris_key].enable_player_specific_items(object_path);
  }

  public void enable_track_specific_items_for_client (string object_path,
                                                      string desktop_id)
  {
    var mpris_key = determine_key ( desktop_id );
    if (this.registered_clients.has_key (mpris_key) == false){
      warning ("we don't have a client with desktop id %s registered", desktop_id);
      return;
    }
    this.registered_clients[mpris_key].enable_track_specific_items(object_path);
  }

  private static AppInfo? create_app_info ( string desktop )
  {
    DesktopAppInfo info = new DesktopAppInfo ( desktop );
    if ( desktop == null || info == null ){
      warning ( "Could not create a desktopappinfo instance from app: %s", desktop );
      return null;
    }
    GLib.AppInfo app_info = info as GLib.AppInfo;
    return app_info;
  }
 
  private static string? fetch_icon_name(string desktop)
  {
    // We know the appinfo is good because it was loaded in the previous reg step.
    DesktopAppInfo info = new DesktopAppInfo ( desktop.concat( ".desktop" ) ) ;
    KeyFile desktop_keyfile = new KeyFile ();
    try{
      desktop_keyfile.load_from_file (info.get_filename(), KeyFileFlags.NONE);
    }
    catch(GLib.FileError error){
      warning("Error loading keyfile - FileError");
      return null;
    }
    catch(GLib.KeyFileError error){
      warning("Error loading keyfile - KeyFileError");      
      return null;
    } 
    
    try{
      return desktop_keyfile.get_string (KeyFileDesktop.GROUP,
                                         KeyFileDesktop.KEY_ICON);              
    }
    catch(GLib.KeyFileError error){
      warning("Error trying to fetch the icon name from the keyfile");      
      return null;
    } 
  }

  /*
    Messy but necessary method to consolidate desktop filesnames and mpris dbus
    names into the one single word string (used as the key in the players hash).
    So this means that we can determine the key for the players_hash from the 
    dbus interface name or the desktop file name, at startup offline/online and 
    shutdown.
   */
  private static string? determine_key(owned string desktop_or_interface)
  {
    var result = desktop_or_interface;
    var tokens = desktop_or_interface.split( "." );
    if (tokens != null && tokens.length > 1){
      result = tokens[tokens.length - 1];  
    }
    var temp = result.split("-");
    if (temp != null && temp.length > 1){
      result = temp[0];
    }
    return result;        
  }
  
}




