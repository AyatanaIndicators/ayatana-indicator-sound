using Dbusmenu;
using Gee;
using DbusmenuMetadata;

public class MetadataMenuitem : PlayerItem
{
	public MetadataMenuitem()
  {
		this.property_set(MENUITEM_PROP_TYPE, MENUITEM_TYPE);
	}

	public static HashSet<string> attributes_format()
	{
		HashSet<string> attrs = new HashSet<string>();		
		attrs.add(MENUITEM_TEXT_TITLE);
    attrs.add(MENUITEM_TEXT_ARTIST);
    attrs.add(MENUITEM_TEXT_ALBUM);
    attrs.add(MENUITEM_ARTURL);
		return attrs;
	}
		
}