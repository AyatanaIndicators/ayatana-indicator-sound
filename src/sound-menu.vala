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

class SoundMenu: Object
{
	public SoundMenu (string settings_action) {
		/* A sound menu always has at least two sections: the volume section (this.volume_section)
		 * at the start of the menu, and the settings section at the end. Between those two,
		 * it has a dynamic amount of player sections, one for each registered player.
		 */

		this.volume_section = new Menu ();
		volume_section.append (_("Mute"), "indicator.mute");
		volume_section.append_item (this.create_slider_menu_item ("indicator.volume", 0.0, 1.0, 0.01,
																  "audio-volume-low-zero-panel",
																  "audio-volume-low-high-panel"));

		this.menu = new Menu ();
		this.menu.append_section (null, volume_section);
		this.menu.append (_("Sound Settings…"), settings_action);

		var root_item = new MenuItem (null, "indicator.root");
		root_item.set_attribute ("x-canonical-type", "s", "com.canonical.indicator.root");
		root_item.set_submenu (this.menu);

		this.root = new Menu ();
		root.append_item (root_item);
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
			return this.volume_section.get_n_items () == 3;
		}
		set {
			if (value && this.volume_section.get_n_items () < 3) {
				var slider = this.create_slider_menu_item ("indicator.mic-volume", 0.0, 1.0, 0.01,
														   "audio-input-microphone-low-zero-panel",
														   "audio-input-microphone-high-panel");
				volume_section.append_item (slider);
			}
			else if (!value && this.volume_section.get_n_items () > 2) {
				this.volume_section.remove (2);
			}
		}
	}

	public void add_player (MediaPlayer player) {
		/* Add new players to the end of the player sections, just before the settings */
		var player_item = new MenuItem (player.name, "indicator." + player.id);
		player_item.set_attribute ("x-canonical-type", "s", "com.canonical.unity.media-player");
		player_item.set_attribute_value ("icon", g_icon_serialize (player.icon));

		var playback_item = new MenuItem (null, null);
		playback_item.set_attribute ("x-canonical-type", "s", "com.canonical.unity.playback-item");
		playback_item.set_attribute ("x-canonical-play-action", "s", "indicator.play." + player.id);
		playback_item.set_attribute ("x-canonical-next-action", "s", "indicator.next." + player.id);
		playback_item.set_attribute ("x-canonical-previous-action", "s", "indicator.previous." + player.id);

		var section = new Menu ();
		section.append_item (player_item);
		section.append_item (playback_item);

		player.playlists_changed.connect (this.update_playlists);
		player.notify["is-running"].connect ( () => this.update_playlists (player) );
		update_playlists (player);

		this.menu.insert_section (this.menu.get_n_items () -1, null, section);
	}

	public void remove_player (MediaPlayer player) {
		int index = this.find_player_section (player);
		if (index >= 0)
			this.menu.remove (index);
	}

	Menu root;
	Menu menu;
	Menu volume_section;

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
		player_section.append_submenu ("Choose Playlist", submenu);
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
