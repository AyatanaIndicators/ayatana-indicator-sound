/*
Copyright 2011 Canonical Ltd.

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
using DbusmenuTrackSpecific;

public class TrackSpecificMenuitem : PlayerItem
{
  public TrackSpecificMenuitem (PlayerController parent)
  {
    Object(item_type: MENUITEM_TYPE, owner: parent);    
  }
  construct
  {
    this.property_set_bool (MENUITEM_PROP_VISIBLE, false);
    this.property_set_bool (MENUITEM_PROP_ENABLED, false);    
    this.property_set (MENUITEM_PROP_LABEL, "Like This");
  }
}
