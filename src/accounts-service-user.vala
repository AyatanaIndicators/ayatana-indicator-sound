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
	public abstract bool playing{owned get; set;}
}

public class AccountsServiceUser : Object {
	Act.UserManager accounts_manager = Act.UserManager.get_default();
	Act.User? user = null;
	AccountsServiceSoundSettings? proxy = null;
	MediaPlayer? _player = null;

	public MediaPlayer? player {
		set {
			this._player = value;
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
				"org.freedesktop.AccountsService",
				user.get_object_path(),
				DBusProxyFlags.GET_INVALIDATED_PROPERTIES,
				null,
				new_proxy);
		});

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
