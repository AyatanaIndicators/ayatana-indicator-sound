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


[DBus (name = "org.mpris.MediaPlayer2")]
public interface MprisRoot : Object {
	// properties
	public abstract bool HasTracklist{owned get; set;}
	public abstract bool CanQuit{owned get; set;}
	public abstract bool CanRaise{owned get; set;}
	public abstract string Identity{owned get; set;}
	public abstract string DesktopEntry{owned get; set;}	
	// methods
	public abstract async void Quit() throws IOError;
	public abstract async void Raise() throws IOError;
}

[DBus (name = "org.mpris.MediaPlayer2.Player")]
public interface MprisPlayer : Object {
	// properties
	public abstract HashTable<string, Variant?> Metadata{owned get; set;}
	public abstract int32 Position{owned get; set;}
	public abstract string PlaybackStatus{owned get; set;}	
	// methods
	public abstract async void PlayPause() throws IOError;
	public abstract async void Next() throws IOError;
	public abstract async void Previous() throws IOError;
	// signals
	public signal void Seeked(int64 new_position);
}
