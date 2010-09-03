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
using Gdk;

public class PlayerItem : Dbusmenu.Menuitem
{
	public PlayerController owner {get; construct;}
	public string item_type { get; construct; }
	private const int EMPTY = -1;
	private FetchFile fetcher;
	private string previous_temp_album_art_path;

	public PlayerItem(string type)
	{		
		Object(item_type: type);
	}
	
	construct {
		this.property_set(MENUITEM_PROP_TYPE, item_type);
		this.previous_temp_album_art_path = null;
	}

	public void reset(HashSet<string> attrs){		
		foreach(string s in attrs){
			debug("attempting to set prop %s to EMPTY", s);
			this.property_set_int(s, EMPTY);
		}
	}
	
	/**
	 * update()
	 * Base update method for playeritems, takes the attributes and the incoming updates
	 * and attmepts to update the appropriate props on the object. 
	 * Album art is handled separately to deal with remote and local file paths.
	 */
	public void update(HashTable<string, Value?> data, HashSet<string> attributes)
	{
		debug("PlayerItem::update()");
		if(data == null){
			debug("PlayerItem::Update -> The hashtable was null - just leave it!");
			return;
		}		
		
		foreach(string property in attributes){
			string[] input_keys = property.split("-");
			string search_key = input_keys[input_keys.length-1 : input_keys.length][0];
			debug("search key = %s", search_key);
			Value? v = data.lookup(search_key);
			
			if (v.holds (typeof (string))){
				string update = v.get_string().strip();
				debug("with value : %s", update);
				if(property.contains("mpris:artUrl")){
					  this.fetch_art(update.strip(), property);
					 	continue;					                     
				}
				this.property_set(property, update);											
			}			    
			else if (v.holds (typeof (int))){
				debug("with value : %i", v.get_int());
				this.property_set_int(property, v.get_int());
			}
			else if (v.holds (typeof (int64))){
				debug("with value : %i", (int)v.get_int64());
				this.property_set_int(property, (int)v.get_int64());
			}
			else if(v.holds (typeof (bool))){
				debug("with value : %s", v.get_boolean().to_string());				
				this.property_set_bool(property, v.get_boolean());
			}
		}
		if(this.property_get_bool(MENUITEM_PROP_VISIBLE) == false){
			this.property_set_bool(MENUITEM_PROP_VISIBLE, true);
		}
	}	
	
	public bool populated(HashSet<string> attrs)
	{
		foreach(string prop in attrs){
			debug("populated ? - prop: %s", prop);
		  int value_int = property_get_int(prop);
			debug("populated ? - prop %s and value %i", prop, value_int);
			if(property_get_int(prop) != EMPTY){
				return true;
			}
		}
		return false;
	}

	public void fetch_art(string uri, string prop)
	{		
		File art_file = File.new_for_uri(uri);
		if(art_file.is_native() == true){
			string path;		
			try{
				path = Filename.from_uri(uri.strip());			
				this.property_set(prop, path);			
			}
			catch(ConvertError e){
				warning("Problem converting URI %s to file path",
					      uri); 
			}
			// eitherway return, the artwork was local
			return;			
		}
		// otherwise go remote
		this.fetcher = new FetchFile (uri, prop);
		this.fetcher.failed.connect (() => { this.on_fetcher_failed ();});
		this.fetcher.completed.connect (this.on_fetcher_completed);
		this.fetcher.fetch_data ();		
	}
	
	private void on_fetcher_failed ()
	{
		warning("on_fetcher_failed -> could not fetch artwork");	
	}

	private void on_fetcher_completed(ByteArray update, string property)
	{
		try{
			PixbufLoader loader = new PixbufLoader ();
			loader.write (update.data, update.len);
			loader.close ();
			Pixbuf icon = loader.get_pixbuf ();				
 			string path = Environment.get_user_special_dir(UserDirectory.PICTURES).dup().concat("/indicator-sound-XXXXXX");
			int r = FileUtils.mkstemp(path);		
			icon.save (path, loader.get_format().get_name());		
			if(this.previous_temp_album_art_path != null){
				FileUtils.remove(this.previous_temp_album_art_path);
			}			
			this.previous_temp_album_art_path = path;
			this.property_set(property, path);
		}
	  catch(GLib.Error e){
			warning("Problem creating file from bytearray fetched from the interweb - error: %s",
			        e.message);
		}				
	}		
	
}

