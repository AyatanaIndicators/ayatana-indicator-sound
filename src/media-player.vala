/*
 * Copyright 2013 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *      Lars Uebernickel <lars.uebernickel@canonical.com>
 */

/**
 * MediaPlayer represents an MRPIS-capable media player.
 */
public class MediaPlayer: Object {

	public MediaPlayer (DesktopAppInfo appinfo) {
		this.appinfo = appinfo;
	}

	/** Desktop id of the player */
	public string id {
		get {
			return this.appinfo.get_id ();
		}
	}

	/** Display name of the player */
	public string name {
		get {
			return this.appinfo.get_name ();
		}
	}

	/** Application icon of the player */
	public Icon icon {
		get {
			return this.appinfo.get_icon ();
		}
	}

	/**
	 * True if an instance of the player is currently running.
	 *
	 * See also: attach(), detach()
	 */
	public bool is_running {
		get {
			return this.proxy != null;
		}
	}

	/** Name of the player on the bus, if an instance is currently running */
	public string dbus_name {
		get {
			return this._dbus_name;
		}
	}

	/**
	 * Attach this object to a process of the associated media player.  The player must own @dbus_name and
	 * implement the org.mpris.MediaPlayer2.Player interface.
	 *
	 * Only one player can be attached at any given time.  Use detach() to detach a player.
	 *
	 * This method does not block.  If it is successful, "is-running" will be set to %TRUE.
	 */
	public void attach (string dbus_name) {
		return_if_fail (this._dbus_name == null && this.proxy == null);

		this._dbus_name = dbus_name;
		Bus.get_proxy.begin<MprisPlayer> (BusType.SESSION, dbus_name, "/org/mpris/MediaPlayer2",
										  DBusProxyFlags.NONE, null, got_proxy);
	}

	/**
	 * Detach this object from a process running the associated media player.
	 *
	 * See also: attach()
	 */
	public void detach () {
		this.proxy = null;
		this._dbus_name = null;
		this.notify_property ("is-running");
	}

	/**
	 * Launch the associated media player.
	 *
	 * Note: this will _not_ call attach(), because it doesn't know on which dbus-name the player will appear.
	 * Use attach() to attach this object to a running instance of the player.
	 */
	public void launch () {
		try {
			this.appinfo.launch (null, null);
		}
		catch (Error e) {
			warning ("unable to launch %s: %s", appinfo.get_name (), e.message);
		}
	}

	DesktopAppInfo appinfo;
	MprisPlayer? proxy;
	string _dbus_name;

	void got_proxy (Object? obj, AsyncResult res) {
		try {
			this.proxy = Bus.get_proxy.end (res);
			this.notify_property ("is-running");
		}
		catch (Error e) {
			this._dbus_name = null;
			warning ("unable to attach to media player: %s", e.message);
		}
	}
}
