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
using Gee;

public class SettingsManager : GLib.Object
{
  private Settings settings;
  public signal void blacklist_updates ( string[] new_blacklist );
  
  public SettingsManager ( ){
  }
  construct{
    this.settings = new Settings ("com.canonical.indicator.sound");
    this.settings.changed["blacklisted-media-players"].connect (on_blacklist_event);    
  }
   
  public string[] fetch_blacklist()
  {
    return this.settings.get_strv ("blacklisted-media-players");
  }

  public ArrayList<string> fetch_interested()
  {
    var blacklisted = this.settings.get_strv ("blacklisted-media-players");
    var interested = this.settings.get_strv ("interested-media-players");
    var list = new ArrayList<string>();
    foreach(var s in interested){
      if (s == "banshee-1"){
        s = "banshee";
      }
      if (s in list) continue;
      if (s in blacklisted) continue;
      list.add(s);
    }
    return list;
  }

  public void clear_list()
  {
    this.settings.reset("interested-media-players");
  }
  
  public void remove_interested (string app_desktop_name)
  {
    var already_interested = this.settings.get_strv ("interested-media-players");
    var list = new ArrayList<string>();

    foreach (var s in already_interested){
      if (s == app_desktop_name) continue;
      list.add (s);
    }
    this.settings.set_strv("interested-media-players",
                            list.to_array());
    this.settings.apply();    
  }
  
  public void add_interested (string app_desktop_name)
  {
    var already_interested = this.settings.get_strv ("interested-media-players");
    foreach (var s in already_interested){
      if ( s == app_desktop_name ) return;
    }
    already_interested += (app_desktop_name);
    this.settings.set_strv( "interested-media-players",
                             already_interested );
    this.settings.apply();
  }

  private void on_blacklist_event()
  {
    this.blacklist_updates(this.settings.get_strv ("blacklisted-media-players"));        
  }

  // Convenient debug method inorder to provide visability over 
  // the contents of both interested and blacklisted containers in its gsettings
/**
  private void reveal_contents()
  {
    var already_interested = this.settings.get_strv ("interested-media-players");
    foreach (var s in already_interested)
    {
      debug ("client %s is in interested array", s);      
    }
    var blacklisted = this.settings.get_strv ("blacklisted-media-players");
    foreach (var s in blacklisted)
    {
      debug ("client %s is in blacklisted array", s);      
    }

    debug ("interested array size = %i", already_interested.length);
    debug ("blacklisted array size = %i", blacklisted.length);
  }
**/
}
