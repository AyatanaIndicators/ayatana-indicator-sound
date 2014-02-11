/*
 * Copyright 2014 © Canonical Ltd.
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

[DBus (name = "com.canonical.indicator.sound.AccountsService")]
public interface AccountsServiceSoundSettings : Object {
	// properties
	public abstract string player_name {owned get; set;}
	public abstract Variant player_icon {owned get; set;}
	public abstract bool running {owned get; set;}
	public abstract string state {owned get; set;}
	public abstract string title {owned get; set;}
	public abstract string artist {owned get; set;}
	public abstract string album {owned get; set;}
	public abstract string art_url {owned get; set;}
}

public class AccountsServiceUser : Object {
	Act.UserManager accounts_manager = Act.UserManager.get_default();
	Act.User? user = null;
	AccountsServiceSoundSettings? proxy = null;
	MediaPlayer? _player = null;

	public MediaPlayer? player {
		set {
			this._player = value;

			/* No proxy, no settings to set */
			if (this.proxy == null)
				return;

			if (this._player == null) {
				/* Clear it */
				this.proxy.player_name = "";
			} else {
				this.proxy.player_name = this._player.name;
				if (this._player.icon == null) {
					var icon = new ThemedIcon.with_default_fallbacks ("application-default-icon");
					this.proxy.player_icon = icon.serialize();
				} else {
					this.proxy.player_icon = this._player.icon.serialize();
				}

				this.proxy.running = this._player.is_running;
				this.proxy.state = this._player.state;

				if (this._player.current_track != null) {
					this.proxy.title = this._player.current_track.title;
					this.proxy.artist = this._player.current_track.artist;
					this.proxy.album = this._player.current_track.album;
					this.proxy.art_url = this._player.current_track.art_url;
				} else {
					this.proxy.title = "";
					this.proxy.artist = "";
					this.proxy.album = "";
					this.proxy.art_url = "";
				}
			}
		}
		get {
			return this._player;
		}
	}

	public AccountsServiceUser () {
		user = accounts_manager.get_user(GLib.Environment.get_user_name());
		user.notify["is-loaded"].connect(() => {
			debug("User loaded");

			this.proxy = null;

			Bus.get_proxy.begin<AccountsServiceSoundSettings> (
				BusType.SYSTEM,
				"org.freedesktop.Accounts",
				user.get_object_path(),
				DBusProxyFlags.GET_INVALIDATED_PROPERTIES,
				null,
				new_proxy);
		});
	}

	~AccountsServiceUser () {
		this.player = null;
	}

	void new_proxy (GLib.Object? obj, AsyncResult res) {
		try {
			this.proxy = Bus.get_proxy.end (res);
			//this.player = _player;
		} catch (Error e) {
			this.proxy = null;
			warning("Unable to get proxy to user sound settings: %s", e.message);
		}
	}
}
