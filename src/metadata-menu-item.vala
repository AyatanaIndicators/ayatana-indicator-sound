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
using DbusmenuMetadata;
using Gdk;

public class MetadataMenuitem : PlayerItem
{
	public const string ALBUM_ART_DIR_SUFFIX = "/indicators/sound/album-art-cache"; 
	
	public static string album_art_cache_dir;
	private static FetchFile fetcher;
	private string previous_temp_album_art_path;
	
	public MetadataMenuitem()
  {
		Object(item_type: MENUITEM_TYPE); 	
		reset(attributes_format());
	}
	
	construct{
		MetadataMenuitem.clean_album_art_temp_dir();
		this.previous_temp_album_art_path = null;		
		this.album_art_cache_dir = MetadataMenuitem.create_album_art_temp_dir();
	}

	private static void clean_album_art_temp_dir()
	{
		string path = Environment.get_user_cache_dir().dup().concat(ALBUM_ART_DIR_SUFFIX);

		GLib.File? album_art_dir = GLib.File.new_for_uri(path);

		/*if(album_art_dir == null || album_art_dir.query_exists(null) == false){
			warning("here %s %s", (album_art_dir.query_exists(null) == false).to_string(), path);
			return;
		}
		*/
		
		if(delete_album_art_contents(album_art_dir) == false)
		{
			warning("could not remove the temp album art files %s", path);
		}
	}

	private static string? create_album_art_temp_dir()
	{
		string path = Environment.get_user_cache_dir().dup().concat(ALBUM_ART_DIR_SUFFIX); 
		if(DirUtils.create(path, 0700) == -1){
			warning("could not create a temp dir for remote album art, it must have been created already");
		}
		return path;
	}
	
	private static bool delete_album_art_contents (GLib.File dir)
  {
		bool result = true;
    try {
      var e = dir.enumerate_children (FILE_ATTRIBUTE_STANDARD_NAME + "," + FILE_ATTRIBUTE_STANDARD_TYPE ,
                                      FileQueryInfoFlags.NOFOLLOW_SYMLINKS,
                                      null);
      while (true)
        {
          var file = e.next_file (null);

					debug("file name = %s", file.get_name());
					
          if (file == null)					
            break;

          var child = dir.get_child (file.get_name ());

            try {
              child.delete (null);
            } catch (Error error_) {
              warning (@"Unable to delete file '$(child.get_basename ()): $(error_.message)");
							result = false;
            }
        }
    } catch (Error error) {
      warning (@"Unable to read files from directory '$(dir.get_basename ())': %s",
               error.message);
			result = false;
    }
		return result;
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
		// If we didn't manage to create the temp dir
		// don't bother with remote
		debug("fetch_art -remotely %s", this.album_art_cache_dir);

		if(this.album_art_cache_dir == null){
			return;
		}
		// green light to go remote		
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
 			string path = this.album_art_cache_dir.concat("/XXXXXX");
			int r = FileUtils.mkstemp(path);
			if(r != -1){
				icon.save (path, loader.get_format().get_name());		
				this.property_set(property, path);
				if(this.previous_temp_album_art_path != null){
					FileUtils.remove(this.previous_temp_album_art_path);
				}			
				this.previous_temp_album_art_path = path;				
			}				
		}
	  catch(GLib.Error e){
			warning("Problem creating file from bytearray fetched from the interweb - error: %s",
			        e.message);
		}				
	}			
	
	public static HashSet<string> attributes_format()
	{
		HashSet<string> attrs = new HashSet<string>();		
		attrs.add(MENUITEM_TITLE);
    attrs.add(MENUITEM_ARTIST);
    attrs.add(MENUITEM_ALBUM);
    attrs.add(MENUITEM_ARTURL);
		return attrs;
	}	
}
