using Dbusmenu;
using Gee;
using DbusmenuMetadata;

public class MetadataMenuitem : PlayerItem
{
	public MetadataMenuitem()
  {
		this.property_set(MENUITEM_PROP_TYPE, MENUITEM_TYPE);
	}

	//public override void update(HashMap<string, string> data)
	//{
	//	this.property_set(MENUITEM_TEXT_ARTIST, data.get("artist").strip());
	//	this.property_set(MENUITEM_TEXT_TITLE, data.get("title").strip());
	//	this.property_set(MENUITEM_TEXT_ALBUM, data.get("album").strip());
	//	this.property_set(MENUITEM_IMAGE_PATH, sanitize_image_path(data.get("arturl")));
	//}

	public static string sanitize_image_path(string path)
	{
		string result = path.strip();
		if(result.has_prefix("file:///")){
			result = result.slice(7, result.len());		                   
		}
		debug("Sanitize image path - result = %s", result);
		return result;
	}

	public static HashMap<string, Type> attributes_format()
	{
		HashMap<string,Type> results = new HashMap<string, Type>();		
		results.set(MENUITEM_TEXT_TITLE, typeof(string));
    results.set(MENUITEM_TEXT_ARTIST, typeof(string));
    results.set(MENUITEM_TEXT_ALBUM, typeof(string));
    results.set(MENUITEM_TEXT_ARTURL, typeof(string));
		return results;
	}
		

	public static HashMap<string, string> format_updates(HashMap<string, Value?> data)
	{
		HashMap<string,string> results = new HashMap<string, string>();		
		
		results.set(MENUITEM_TEXT_TITLE, (string)data.lookup("title").strip());
    results.set(MENUITEM_TEXT_ARTIST, (string)data.lookup("artist").strip());
    results.set(MENUITEM_TEXT_ALBUM, (string)data.lookup("album").strip(), typeof(string));
    results.set(MENUITEM_TEXT_ARTURL, sanitize_image_path((string)data.lookup("arturl").strip()), typeof(string));
		return results;
	}
}