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
 * MediaPlayerList is a list of media players that should appear in the sound menu.  Its main responsibility is
 * to listen for MPRIS players on the bus and attach them to the corresponding %Player objects.
 */
public class MediaPlayerList {

	public MediaPlayerList () {
		this._players = new HashTable<string, MediaPlayer> (str_hash, str_equal);

		this.mpris_watcher = new Mpris2Watcher ();
		this.mpris_watcher.client_appeared.connect (this.player_appeared);
		this.mpris_watcher.client_disappeared.connect (this.player_disappeared);
	}

	public List<MediaPlayer> players {
		owned get {
			return this._players.get_values ();
		}
	}

	/**
	 * Adds the player associated with @desktop_id.  Does nothing if such a player already exists.
	 */
	public MediaPlayer? insert (string desktop_id) {
		message ("inserting %s", desktop_id);
		MediaPlayer? player = this._players.lookup (desktop_id);

		if (player == null) {
			message ("  Really.");
			var appinfo = new DesktopAppInfo (desktop_id.has_suffix (".desktop") ? desktop_id : desktop_id + ".desktop");
			if (appinfo == null) {
				warning ("unable to find application '%s'", desktop_id);
				return null;
			}

			player = new MediaPlayer (appinfo);
			this._players.insert (player.id, player);
			this.player_added (player);
		}

		return player;
	}

	/**
	 * Removes the player associated with @desktop_id, unless it is currently running.
	 */
	public void remove (string desktop_id) {
		MediaPlayer? player = this._players.lookup (desktop_id);

		if (player != null && !player.is_running) {
			this._players.remove (desktop_id);
			this.player_removed (player);
		}
	}

	/**
	 * Synchronizes the player list with @desktop_ids.  After this call, this list will only contain the players
	 * in @desktop_ids.  Players that were running but are not in @desktop_ids will remain in the list.
	 */
	public void sync (string[] desktop_ids) {

		/* hash desktop_ids for faster lookup */
		var hash = new HashTable<string, unowned string> (str_hash, str_equal);
		foreach (var id in desktop_ids)
			hash.add (id);

		/* remove players that are not desktop_ids */
		foreach (var id in this._players.get_keys ()) {
			if (!hash.contains (id))
				this.remove (id);
		}

		/* insert all players (insert() takes care of not adding a player twice */
		foreach (var id in desktop_ids)
			this.insert (id);
	}

	public signal void player_added (MediaPlayer player);
	public signal void player_removed (MediaPlayer player);

	HashTable<string, MediaPlayer> _players;
	Mpris2Watcher mpris_watcher;

	void player_appeared (string desktop_id, string dbus_name, bool use_playlists) {
		var player = this.insert (desktop_id);
		if (player != null)
			player.attach (dbus_name);
	}

	void player_disappeared (string dbus_name) {
		MediaPlayer? player = this._players.find ( (name, player) => {
			return player.dbus_name == dbus_name;
		});

		if (player != null)
			player.detach ();
	}
}
