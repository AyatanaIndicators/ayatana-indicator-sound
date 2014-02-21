/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *      Ted Gould <ted@canonical.com>
 */

public abstract class MediaPlayerUser : MediaPlayer {
	string username;

	MediaPlayerUser(string user) {
		username = user;
	}

	public override string id {
		get { return username; }
	}

	public override string name { 
		get {
			/* TODO */
			return "";
		}
	}
	public override string state {
		get {
			/* TODO */
			return "";
		}
		set { }
	}
	public override Icon? icon { 
		get { 
			/* TODO */
			return null;
		}
	}
	public override string dbus_name { get { return ""; } }

	public override bool is_running { get { return true; } }
	public override bool can_raise { get { return false; } }

	public override MediaPlayer.Track? current_track {
		get { 
			/* TODO: */
			return null;
		}
		set { }
	}

	/* Control functions through unity-greeter-session-broadcast */
	public override void activate () {
		/* TODO: */
	}
	public override void play_pause () {
		/* TODO: */
	}
	public override void next () {
		/* TODO: */
	}
	public override void previous () {
		/* TODO: */
	}

	/* Play list functions are all null as we don't support the
	   playlist feature on the greeter */
	public override uint get_n_playlists() {
		return 0;
	}
	public override string get_playlist_id (int index) {
		return "";
	}
	public override string get_playlist_name (int index) {
		return "";
	}
	public override void activate_playlist_by_name (string playlist) {
	}
}
