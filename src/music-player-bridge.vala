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
  private SettingsManager settings_manager;
  private Dbusmenu.Menuitem root_menu;
  private HashMap<string, PlayerController> registered_clients;  
  private Mpris2Watcher watcher;
  private const string DESKTOP_PREFIX = "/usr/share/applications/";
  private Settings settings;
  
  public MusicPlayerBridge()
  {
  }
  
  construct{
    this.registered_clients = new HashMap<string, PlayerController> ();
    this.settings_manager = new SettingsManager();
    this.settings_manager.blacklist_updates.connect ( this.on_blacklist_update );
  }
  
  private void on_blacklist_update ( string[] blacklist )
  {
    debug("some blacklist update");
  }

  private void try_to_add_inactive_familiar_clients()
  {
    foreach ( string desktop in this.settings_manager.fetch_interested()){
      debug ( "interested client found : %s", desktop );
      string path = DESKTOP_PREFIX.concat ( desktop.concat( ".desktop" ) );
      AppInfo? app_info = create_app_info ( path );
      if ( app_info == null ){
        warning ( "Could not create app_info for path %s \n Getting out of here ", path);
        continue;
      }
      var mpris_key = determine_key ( path );
      PlayerController ctrl = new PlayerController ( this.root_menu, 
                                                     app_info,
                                                     null,
                                                     this.fetch_icon_name(path),
                                                     calculate_menu_position(),
                                                     PlayerController.state.OFFLINE );
      this.registered_clients.set(mpris_key, ctrl);  
    }
  }

  private int calculate_menu_position()
  {
    if(this.registered_clients.size == 0){
      return 2;
    }
    else{
      return (2 + (this.registered_clients.size * PlayerController.WIDGET_QUANTITY));
    }
  }

  public void  client_has_become_available ( string desktop, string dbus_name )
  {
    if (desktop == null || desktop == ""){
      warning("Client %s attempting to register without desktop entry being set on the mpris root",
               dbus_name);
      return;
    }
    debug ( "client_has_become_available %s", desktop );
    string path = DESKTOP_PREFIX.concat ( desktop.concat( ".desktop" ) );
    AppInfo? app_info = create_app_info ( path );
    if ( app_info == null ){
      warning ( "Could not create app_info for path %s \n Getting out of here ", path);
      return;
    }
    
    var mpris_key = determine_key ( path );
    // Are we sure clients will appear like this with the new registration method in place. 
    if ( this.registered_clients.has_key (mpris_key) == false ){
      debug("New client has registered that we have not seen before: %s", dbus_name );
      PlayerController ctrl = new PlayerController ( this.root_menu,
                                                     app_info,
                                                     dbus_name,
                                                     this.fetch_icon_name(path),                                                    
                                                     this.calculate_menu_position(),
                                                     PlayerController.state.READY );
      this.registered_clients.set ( mpris_key, ctrl );
      debug ( "Have not seen this %s before, new controller created.", desktop );        
      this.settings_manager.add_interested ( desktop );
      debug ( "application added to the interested list" );
    }
    else{
      this.registered_clients[mpris_key].update_state ( PlayerController.state.READY );
      this.registered_clients[mpris_key].activate ( dbus_name );
      debug("Application has already registered - awaken the hibernation: %s \n", dbus_name );
    }
  }
  
  public void client_has_vanished ( string mpris_root_interface )
  {
    debug("MusicPlayerBridge -> on_server_removed with value %s", mpris_root_interface);
    if (root_menu != null){
      debug("attempt to remove %s", mpris_root_interface);
      var mpris_key = determine_key ( mpris_root_interface );
      if ( mpris_key != null ){
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
    this.watcher.client_appeared += this.client_has_become_available;
    this.watcher.client_disappeared += this.client_has_vanished;
  }

  private static AppInfo? create_app_info ( string path )
  {
    DesktopAppInfo info = new DesktopAppInfo.from_filename ( path ) ;
    if ( path == null || info == null ){
      warning ( "Could not create a desktopappinfo instance from app: %s", path );
      return null;
    }
    GLib.AppInfo app_info = info as GLib.AppInfo;
    return app_info;
  }
 
  private static string? fetch_icon_name(string desktop_path)
  {
    KeyFile desktop_keyfile = new KeyFile ();
    try{
      desktop_keyfile.load_from_file (desktop_path, KeyFileFlags.NONE);
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
    Messy but necessary method to consolidate desktop filesnames and mpris dbus names
    into the one single word string (used as the key in the players hash).
    So this means that we can determine the key for the players_hash from the 
    dbus interface name or the desktop file name.
   */
  private static string? determine_key(owned string path)
  {
    var tokens = path.split( "/" );
    if ( tokens.length < 2 ){
      // try to split on "."
      tokens = path.split(".");
      if ( tokens.length < 2 ){
        // don't know what this is
        return null;
      }
    }
    var filename = tokens[tokens.length - 1];
    var result = filename.split(".")[0];
    var temp = result.split("-");
    if (temp.length > 1){
      result = temp[0];
    }
    debug("determine key result = %s", result);
    return result;        
  }
  
}




