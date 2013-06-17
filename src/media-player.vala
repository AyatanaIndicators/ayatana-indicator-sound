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

	public string state {
		get; private set; default = "Paused";
	}

	public class Track : Object {
		public string artist { get; construct; }
		public string title { get; construct; }
		public string album { get; construct; }
		public string art_url { get; construct; }

		public Track (string artist, string title, string album, string art_url) {
			Object (artist: artist, title: title, album: album, art_url: art_url);
		}
	}

	public Track current_track {
		get; set;
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
										  DBusProxyFlags.GET_INVALIDATED_PROPERTIES, null, got_proxy);
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
		this.state = "Paused";
		this.current_track = null;
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
		this.state = "Launching";
	}

	/**
	 * Toggles playing status.
	 */
	public void play_pause () {
		if (this.proxy != null)
			this.proxy.PlayPause.begin ();
	}

	/**
	 * Skips to the next track.
	 */
	public void next () {
		if (this.proxy != null)
			this.proxy.Next.begin ();
	}

	/**
	 * Skips to the previous track.
	 */
	public void previous () {
		if (this.proxy != null)
			this.proxy.Previous.begin ();
	}

	DesktopAppInfo appinfo;
	MprisPlayer? proxy;
	string _dbus_name;

	void got_proxy (Object? obj, AsyncResult res) {
		try {
			this.proxy = Bus.get_proxy.end (res);

			/* Connecting to GDBusProxy's "g-properties-changed" signal here, because vala's dbus objects don't
			 * emit notify signals */
			var gproxy = this.proxy as DBusProxy;
			gproxy.g_properties_changed.connect (this.proxy_properties_changed);

			this.notify_property ("is-running");
			this.state = this.proxy.PlaybackStatus;
			this.update_current_track (gproxy.get_cached_property ("Metadata"));
		}
		catch (Error e) {
			this._dbus_name = null;
			warning ("unable to attach to media player: %s", e.message);
		}
	}

	/* some players (e.g. Spotify) don't follow the spec closely and pass single strings in metadata fields
	 * where an array of string is expected */
	static string sanitize_metadata_value (Variant? v) {
		if (v == null)
			return "";
		else if (v.is_of_type (VariantType.STRING))
			return v.get_string ();
		else if (v.is_of_type (VariantType.STRING_ARRAY))
			return string.joinv (",", v.get_strv ());

		warn_if_reached ();
		return "";
	}

	void proxy_properties_changed (DBusProxy proxy, Variant changed_properties, string[] invalidated_properties) {
		if (changed_properties.lookup ("PlaybackStatus", "s", null)) {
			this.state = this.proxy.PlaybackStatus;
		}

		var metadata = changed_properties.lookup_value ("Metadata", new VariantType ("a{sv}"));
		if (metadata != null)
			this.update_current_track (metadata);
	}

	void update_current_track (Variant metadata) {
		if (metadata != null) {
			this.current_track = new Track (
				sanitize_metadata_value (metadata.lookup_value ("xesam:artist", null)),
				sanitize_metadata_value (metadata.lookup_value ("xesam:title", null)),
				sanitize_metadata_value (metadata.lookup_value ("xesam:album", null)),
				sanitize_metadata_value (metadata.lookup_value ("mpris:artUrl", null))
			);
		}
		else {
			this.current_track = null;
		}
	}
}
