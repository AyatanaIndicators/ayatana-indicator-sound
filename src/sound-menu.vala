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

/* Icon.serialize() is not yet in gio-2.0.vapi; remove this when it is */
extern Variant? g_icon_serialize (Icon icon);

class SoundMenu: Object
{
	public enum DisplayFlags {
		NONE = 0,
		SHOW_MUTE = 1,
		HIDE_INACTIVE_PLAYERS = 2
	}

	public SoundMenu (string? settings_action, DisplayFlags flags) {
		/* A sound menu always has at least two sections: the volume section (this.volume_section)
		 * at the start of the menu, and the settings section at the end. Between those two,
		 * it has a dynamic amount of player sections, one for each registered player.
		 */

		this.volume_section = new Menu ();
		if ((flags & DisplayFlags.SHOW_MUTE) != 0)
			volume_section.append (_("Mute"), "indicator.mute");
		volume_section.append_item (this.create_slider_menu_item ("indicator.volume", 0.0, 1.0, 0.01,
																  "audio-volume-low-zero-panel",
																  "audio-volume-high-panel"));

		this.menu = new Menu ();
		this.menu.append_section (null, volume_section);

		if (settings_action != null) {
			settings_shown = true;
			this.menu.append (_("Sound Settings…"), settings_action);
		}

		var root_item = new MenuItem (null, "indicator.root");
		root_item.set_attribute ("x-canonical-type", "s", "com.canonical.indicator.root");
		root_item.set_attribute ("x-canonical-scroll-action", "s", "indicator.scroll");
		root_item.set_attribute ("x-canonical-secondary-action", "s", "indicator.mute");
		root_item.set_submenu (this.menu);

		this.root = new Menu ();
		root.append_item (root_item);

		this.hide_inactive = (flags & DisplayFlags.HIDE_INACTIVE_PLAYERS) != 0;
		this.notify_handlers = new HashTable<MediaPlayer, ulong> (direct_hash, direct_equal);
	}

	public void export (DBusConnection connection, string object_path) {
		try {
			connection.export_menu_model (object_path, this.root);
		} catch (Error e) {
			critical ("%s", e.message);
		}
	}

	public bool show_mic_volume {
		get {
			return this.mic_volume_shown;
		}
		set {
			if (value && !this.mic_volume_shown) {
				var slider = this.create_slider_menu_item ("indicator.mic-volume", 0.0, 1.0, 0.01,
														   "audio-input-microphone-low-zero-panel",
														   "audio-input-microphone-high-panel");
				volume_section.append_item (slider);
				this.mic_volume_shown = true;
			}
			else if (!value && this.mic_volume_shown) {
				this.volume_section.remove (this.volume_section.get_n_items () -1);
				this.mic_volume_shown = false;
			}
		}
	}

	public void add_player (MediaPlayer player) {
		if (this.notify_handlers.contains (player))
			return;

		if (player.is_running || !this.hide_inactive)
			this.insert_player_section (player);
		this.update_playlists (player);

		var handler_id = player.notify["is-running"].connect ( () => {
			if (this.hide_inactive) {
				if (player.is_running)
					this.insert_player_section (player);
				else
					this.remove_player_section (player);
			}
			this.update_playlists (player);
		});
		this.notify_handlers.insert (player, handler_id);

		player.playlists_changed.connect (this.update_playlists);
	}

	public void remove_player (MediaPlayer player) {
		this.remove_player_section (player);
		this.notify_handlers.remove (player);
	}

	Menu root;
	Menu menu;
	Menu volume_section;
	bool mic_volume_shown;
	bool settings_shown = false;
	bool hide_inactive;
	HashTable<MediaPlayer, ulong> notify_handlers;

	/* returns the position in this.menu of the section that's associated with @player */
	int find_player_section (MediaPlayer player) {
		string action_name = @"indicator.$(player.id)";
		int n = this.menu.get_n_items () -1;
		for (int i = 1; i < n; i++) {
			var section = this.menu.get_item_link (i, Menu.LINK_SECTION);
			string action;
			section.get_item_attribute (0, "action", "s", out action);
			if (action == action_name)
				return i;
		}

		return -1;
	}

	void insert_player_section (MediaPlayer player) {
		var section = new Menu ();

		var player_item = new MenuItem (player.name, "indicator." + player.id);
		player_item.set_attribute ("x-canonical-type", "s", "com.canonical.unity.media-player");
		player_item.set_attribute_value ("icon", g_icon_serialize (player.icon));
		section.append_item (player_item);

		var playback_item = new MenuItem (null, null);
		playback_item.set_attribute ("x-canonical-type", "s", "com.canonical.unity.playback-item");
		playback_item.set_attribute ("x-canonical-play-action", "s", "indicator.play." + player.id);
		playback_item.set_attribute ("x-canonical-next-action", "s", "indicator.next." + player.id);
		playback_item.set_attribute ("x-canonical-previous-action", "s", "indicator.previous." + player.id);
		section.append_item (playback_item);

		/* Add new players to the end of the player sections, just before the settings */
		if (settings_shown) {
			this.menu.insert_section (this.menu.get_n_items () -1, null, section);
		} else {
			this.menu.append_section (null, section);
		}
	}

	void remove_player_section (MediaPlayer player) {
		int index = this.find_player_section (player);
		if (index >= 0)
			this.menu.remove (index);
	}

	void update_playlists (MediaPlayer player) {
		int index = find_player_section (player);
		if (index < 0)
			return;

		var player_section = this.menu.get_item_link (index, Menu.LINK_SECTION) as Menu;

		/* if a section has three items, the playlists menu is in it */
		if (player_section.get_n_items () == 3)
			player_section.remove (2);

		if (!player.is_running)
			return;

		var count = player.get_n_playlists ();
		if (count == 0)
			return;

		var playlists_section = new Menu ();
		for (int i = 0; i < count; i++) {
			var playlist_id = player.get_playlist_id (i);
			playlists_section.append (player.get_playlist_name (i),
									  @"indicator.play-playlist.$(player.id)::$playlist_id");

		}

		var submenu = new Menu ();
		submenu.append_section (null, playlists_section);
		player_section.append_submenu (_("Choose Playlist"), submenu);
	}

	MenuItem create_slider_menu_item (string action, double min, double max, double step, string min_icon_name, string max_icon_name) {
		var min_icon = new ThemedIcon.with_default_fallbacks (min_icon_name);
		var max_icon = new ThemedIcon.with_default_fallbacks (max_icon_name);

		var slider = new MenuItem (null, action);
		slider.set_attribute ("x-canonical-type", "s", "com.canonical.unity.slider");
		slider.set_attribute_value ("min-icon", g_icon_serialize (min_icon));
		slider.set_attribute_value ("max-icon", g_icon_serialize (max_icon));
		slider.set_attribute ("min-value", "d", min);
		slider.set_attribute ("max-value", "d", max);
		slider.set_attribute ("step", "d", step);

		return slider;
	}
}
