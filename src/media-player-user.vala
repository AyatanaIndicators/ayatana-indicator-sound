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
	Act.UserManager accounts_manager = Act.UserManager.get_default();
	string username;
	Act.User? actuser = null;
	AccountsServiceSoundSettings? proxy = null;

	MediaPlayerUser(string user) {
		username = user;

		actuser = accounts_manager.get_user(user);
		actuser.notify["is-loaded"].connect(() => {
			debug("User loaded");

			this.proxy = null;

			Bus.get_proxy.begin<AccountsServiceSoundSettings> (
				BusType.SYSTEM,
				"org.freedesktop.Accounts",
				actuser.get_object_path(),
				DBusProxyFlags.GET_INVALIDATED_PROPERTIES,
				null,
				new_proxy);
		});
	}

	void new_proxy (GLib.Object? obj, AsyncResult res) {
		try {
			this.proxy = Bus.get_proxy.end (res);
			/* TODO: Update settings */
		} catch (Error e) {
			this.proxy = null;
			warning("Unable to get proxy to user '%s' sound settings: %s", username, e.message);
		}
	}

	public override string id {
		get { return username; }
	}

	string name_cache;
	public override string name { 
		get {
			if (this.proxy != null) {
				name_cache = this.proxy.player_name;
				return name_cache;
			} else {
				return "";
			}
		}
	}
	string state_cache;
	public override string state {
		get {
			if (this.proxy != null) {
				state_cache = this.proxy.state;
				return state_cache;
			} else {
				return "";
			}
		}
		set { }
	}
	Icon icon_cache;
	public override Icon? icon { 
		get { 
			if (this.proxy != null) {
				icon_cache = Icon.deserialize(this.proxy.player_icon);
				return icon_cache;
			} else {
				return null;
			}
		}
	}
	public override string dbus_name { get { return ""; } }

	public override bool is_running { get { return true; } }
	public override bool can_raise { get { return false; } }

	MediaPlayer.Track track_cache;
	public override MediaPlayer.Track? current_track {
		get { 
			if (this.proxy != null) {
				track_cache = new MediaPlayer.Track(
					this.proxy.artist,
					this.proxy.title,
					this.proxy.album,
					this.proxy.art_url
				);
				return track_cache;
			} else {
				return null;
			}
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
