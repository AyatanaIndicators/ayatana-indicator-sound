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

public class MetadataMenuitem : PlayerItem
{
	public MetadataMenuitem()
  {
		Object(item_type: MENUITEM_TYPE); 	
		reset(attributes_format());
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
