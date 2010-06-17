using Dbusmenu;
using Gee;

public class MetadataMenuitem : Dbusmenu.Menuitem
{
	/* Not ideal duplicate definition of const - see common-defs/h */
 	const string DBUSMENU_METADATA_MENUITEM_TYPE = "x-canonical-metadata-menu-item";
 	const string DBUSMENU_METADATA_MENUITEM_TEXT_ARTIST = "x-canonical-metadata-text-artist";
 	const string DBUSMENU_METADATA_MENUITEM_TEXT_PIECE = "x-canonical-metadata-text-piece";
 	const string DBUSMENU_METADATA_MENUITEM_TEXT_CONTAINER = "x-canonical-metadata-text-container";
	const string DBUSMENU_METADATA_MENUITEM_IMAGE_PATH = "x-canonical-metadata-image";

	public MetadataMenuitem()
  {
		this.property_set(MENUITEM_PROP_TYPE, DBUSMENU_METADATA_MENUITEM_TYPE);
	}

	public void update(HashMap<string, string> data)
	{
		this.property_set(DBUSMENU_METADATA_MENUITEM_TEXT_ARTIST, data.get("artist").strip());
		this.property_set(DBUSMENU_METADATA_MENUITEM_TEXT_PIECE, data.get("title").strip());
		this.property_set(DBUSMENU_METADATA_MENUITEM_TEXT_CONTAINER, data.get("album").strip());
		this.property_set(DBUSMENU_METADATA_MENUITEM_IMAGE_PATH, sanitize_image_path(data.get("arturl")));
	}

	public static string sanitize_image_path(string path)
	{
		string result = path.strip();
		if(result.has_prefix("file:///")){
			result = result.slice(7, result.len());		                   
		}
		debug("Sanitize image path - result = %s", result);
		return result;
	}
		
	public override void handle_event(string name, GLib.Value input_value, uint timestamp)
	{
		debug("MetadataItem -> handle event caught!");
	}	
}