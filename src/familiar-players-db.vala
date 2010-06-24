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
	private HashMap<string, bool> players_DB;
	private string file_name;
	private string dir_name;
	private KeyFile key_file;
	private uint write_id;
	
	public FamiliarPlayersDB()
	{
		this.write_id = 0;
		this.players_DB = new HashMap<string, bool>();		
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
		if (test(this.file_name, GLib.FileTest.EXISTS)) {
			this.key_file = new KeyFile();
			try{
				if (this.key_file.load_from_file(this.file_name, KeyFileFlags.NONE) == true) {
					return true;
				}
			}
			catch(FileError e){
				warning("FamiliarPlayersDB - error trying to load KeyFile");
			}
		}
		return false;
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
		
	private bool load_data_from_key_file()
	{
		try{
			string[] desktops = this.key_file.get_string_list(GROUP_NAME,
				                                          			KEY_NAME);
				                                          			
			int i = 0;
			while (desktops[i] != null) {
				this.players_DB.set(desktops[i], true);  
			}
			return true;
		}
		catch(FileError error){
			warning("Error loading the Desktop string list");				
			return false;
		}
	}

	private bool write_db()
	{
		KeyFile  keyfile = new KeyFile();
		string[] desktops = {};
		//Set<string> keys = this.players_DB.keys;
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
		catch(Error e){
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

	public void db_add(string desktop)
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
		return this.players_DB.get(desktop);
	}
}