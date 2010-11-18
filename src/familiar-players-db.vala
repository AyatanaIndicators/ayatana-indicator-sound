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
using GLib.Path;
using GLib.DirUtils;
using GLib.FileUtils;
using GLib.Timeout;
using GLib.Environment;

// TODO: more refactoring needed here
public class FamiliarPlayersDB : GLib.Object
{
	private const string GROUP_NAME = "Seen Database";
	private const string KEY_NAME = "DesktopFiles";	
  private const string DEFAULT_APP_DESKTOP = "/usr/share/applications/banshee-1.desktop";
	private HashMap<string, bool> players_DB;
	private string file_name;
	private string dir_name;
	private KeyFile key_file;
	private uint write_id;
	
	public FamiliarPlayersDB()
	{
		this.write_id = 0;
		this.players_DB = new HashMap<string, bool>();	
    if ( !create_key_file() ){
      this.players_DB.set(DEFAULT_APP_DESKTOP, true);
      this.write_db();
    }
    
		this.dir_name = build_filename(get_user_cache_dir(), "indicators", "sound");
		this.file_name = build_filename(this.dir_name, "familiar-players-db.keyfile");
		if(create_key_file() && check_for_keys() && load_data_from_key_file()){
			debug("keyfiles in place and ready for action");			
		}
		else{      
			this.key_file = null;
			warning("FamiliarPlayersDB:: problems loading key file - can't go any further");
		}		
	}

	private bool create_key_file(){
		bool result = false;
		if (test(this.file_name, GLib.FileTest.EXISTS)) {
			this.key_file = new KeyFile();
			try{
				result = this.key_file.load_from_file(this.file_name, KeyFileFlags.NONE);
			}
			catch(GLib.KeyFileError e){
				warning("FamiliarPlayersDB::create_key_file() - KeyFileError");
			}
			catch(GLib.FileError e){
				warning("FamiliarPlayersDB::create_key_file() - FileError");
			}			
		}
		return result;
	}

	private bool check_for_keys(){
		try{
			if(this.key_file.has_key(GROUP_NAME, KEY_NAME) == true){
				return true;
			}
		}
		catch(KeyFileError e){
				return false;			
		}
		warning("Seen DB '%s' does not have key '%s' in group '%s'", this.file_name, KEY_NAME, GROUP_NAME);
		return false;		
	}
		
	private bool load_data_from_key_file(){
		try{
			string[] desktops = this.key_file.get_string_list(GROUP_NAME,
				                                          			KEY_NAME);			
			foreach(string s in desktops){
				this.players_DB.set(s, true);
			}
			return true;
		}
		catch(GLib.KeyFileError error){
			warning("Error loading the Desktop string list");				
			return false;
		}
	}

	private bool write_db()
	{
		KeyFile  keyfile = new KeyFile();
		string[] desktops = {};
		foreach(string key in this.players_DB.keys){
			desktops += key;
		}
		keyfile.set_string_list(GROUP_NAME,
	                          KEY_NAME,
		                        desktops);
		size_t data_length;
		string data = null;
		try{
			data  = keyfile.to_data(out data_length);
		}
		catch(GLib.KeyFileError e){
			warning("Problems dumping keyfile to a string");
			return false;
		}
		
		if(create_with_parents(this.dir_name, 0700) != 0){
			warning("Unable to make directory: %s", this.dir_name);
			return false;
		}

		try{
			if(set_contents(this.file_name, data, (ssize_t)data_length) == false){
				warning("Unable to write out file '%s'", this.file_name);			
			}
		}
		catch(FileError err){
			warning("Unable to write out file '%s'", this.file_name);			
		}
		return true;
	}

	public void insert(string desktop)
	{
		if(already_familiar(desktop) == false){
			if(this.write_id != 0){
				Source.remove(this.write_id);
				this.write_id = 0;
			}
			this.write_id = Timeout.add_seconds(60, write_db);
			this.players_DB.set(desktop.dup(), true);
		}
	}
	
	public bool already_familiar(string desktop)
	{
		debug("playerDB->already_familiar - result %s", this.players_DB.keys.contains(desktop).to_string()); 
		return this.players_DB.keys.contains(desktop);
	}

	public Gee.Set<string> records()
	{
		return this.players_DB.keys;		
	}

  public static string? fetch_icon_name(string desktop_path)
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
  
}