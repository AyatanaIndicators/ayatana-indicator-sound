/*
This service primarily controls PulseAudio and is driven by the sound indicator menu on the panel.
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
using DbusmenuTransport;

public class TransportMenuitem : PlayerItem
{
	
	public TransportMenuitem()
  {
		this.property_set(MENUITEM_PROP_TYPE, MENUITEM_TYPE);
		this.property_set_bool(MENUITEM_STATE, false);
		debug("transport on the vala side");
	}

	public override void handle_event(string name, GLib.Value input_value, uint timestamp)
	{
		debug("handle_event with bool value %s", input_value.get_boolean().to_string());
		this.mpris_adaptor.toggle_playback(input_value.get_boolean());
	}	
}