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
    this.settings = new Settings ("com.canonical.indicators.sound");
    settings.changed["blacklisted-media-players"].connect (on_blacklist_event);    
    this.fetch_entries.begin();
  }
   
  public string[] fetch_blacklist()
  {
    return this.blacklist_updates(this.settings.get_strv ("blacklisted-media-players"));
  }

  public string[] fetch_interested()
  {
    return this.interested_updates(this.settings.get_strv ("interested-media-players"));
  }

  public bool add_interested(string app_desktop_name)
  {
    string[] already_interested = fetch_interested();
    already_interested.append ( app_desktop_name );
    return this.settings.set_strv( already_interested );
  }

  private on_blacklist_event()
  {
    this.blacklist_updates(this.settings.get_strv ("blacklisted-media-players"));        
  }  
}