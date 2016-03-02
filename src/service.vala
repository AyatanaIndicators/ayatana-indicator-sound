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
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *      Lars Uebernickel <lars.uebernickel@canonical.com>
 */

public class IndicatorSound.Service: Object {
	DBusConnection bus;

	public Service (MediaPlayerList playerlist, VolumeControl volume, AccountsServiceUser? accounts, Options options, VolumeWarning volume_warning, AccountsServiceAccess? accounts_service_access) {

		_accounts_service_access = accounts_service_access;

		try {
			bus = Bus.get_sync(GLib.BusType.SESSION);
		} catch (GLib.Error e) {
			error("Unable to get DBus session bus: %s", e.message);
		}

		_options = options;
		_options.notify["max-volume"].connect(() => {
			update_volume_action_state();
			this.update_notification();
		});

		_volume_warning = volume_warning;
		_volume_warning.notify["active"].connect(() => {
			this.increment_volume_sync_action();
			this.update_notification();
		});

		this.settings = new Settings ("com.canonical.indicator.sound");

		this.settings.bind ("visible", this, "visible", SettingsBindFlags.GET);
		this.notify["visible"].connect ( () => this.update_root_icon () );

		this.volume_control = volume;
		this.volume_control.active_output_changed.connect(() => {
			bool headphones;
			switch(volume_control.active_output()) {
				case VolumeControl.ActiveOutput.HEADPHONES:
				case VolumeControl.ActiveOutput.USB_HEADPHONES:
				case VolumeControl.ActiveOutput.HDMI_HEADPHONES:
				case VolumeControl.ActiveOutput.BLUETOOTH_HEADPHONES:
					headphones = true;
					break;

				default:
					headphones = false;
					break;
			}
			_volume_warning.headphones_active = headphones;

			update_root_icon();
			update_notification();
		});

		this.accounts_service = accounts;
		/* If we're on the greeter, don't export */
		if (this.accounts_service != null) {
			this.accounts_service.notify["showDataOnGreeter"].connect(() => {
				this.export_to_accounts_service = this.accounts_service.showDataOnGreeter;
				eventually_update_player_actions();
			});

			this.export_to_accounts_service = this.accounts_service.showDataOnGreeter;
		}

		this.players = playerlist;
		this.players.player_added.connect (this.player_added);
		this.players.player_removed.connect (this.player_removed);

		this.actions = new SimpleActionGroup ();
		this.actions.add_action_entries (action_entries, this);
		this.actions.add_action (this.create_silent_mode_action ());
		this.actions.add_action (this.create_mute_action ());
		this.actions.add_action (this.create_volume_action ());
		this.actions.add_action (this.create_mic_volume_action ());
		this.actions.add_action (this.create_high_volume_action ());
		this.actions.add_action (this.create_volume_sync_action ());

		this.menus = new HashTable<string, SoundMenu> (str_hash, str_equal);
		this.menus.insert ("desktop_greeter", new SoundMenu (null, SoundMenu.DisplayFlags.SHOW_MUTE | SoundMenu.DisplayFlags.HIDE_PLAYERS | SoundMenu.DisplayFlags.GREETER_PLAYERS));
		this.menus.insert ("phone_greeter", new SoundMenu (null, SoundMenu.DisplayFlags.SHOW_SILENT_MODE | SoundMenu.DisplayFlags.HIDE_INACTIVE_PLAYERS | SoundMenu.DisplayFlags.GREETER_PLAYERS));
		this.menus.insert ("desktop", new SoundMenu ("indicator.desktop-settings", SoundMenu.DisplayFlags.SHOW_MUTE | SoundMenu.DisplayFlags.HIDE_INACTIVE_PLAYERS_PLAY_CONTROLS | SoundMenu.DisplayFlags.ADD_PLAY_CONTROL_INACTIVE_PLAYER));
		this.menus.insert ("phone", new SoundMenu ("indicator.phone-settings", SoundMenu.DisplayFlags.SHOW_SILENT_MODE | SoundMenu.DisplayFlags.HIDE_INACTIVE_PLAYERS));

		this.menus.@foreach ( (profile, menu) => {
			this.volume_control.bind_property ("active-mic", menu, "show-mic-volume", BindingFlags.SYNC_CREATE);
		});

		this.menus.@foreach ( (profile, menu) => {
			_volume_warning.bind_property ("high-volume", menu, "show-high-volume-warning", BindingFlags.SYNC_CREATE);
		});

		this.menus.@foreach ( (profile, menu) => {
			this.volume_control.active_output_changed.connect (menu.update_volume_slider);
		});

		this.menus.@foreach ( (profile, menu) => {
			menu.last_player_updated.connect ((player_id) => { 
				this._accounts_service_access.last_running_player = player_id;
			});
		});

		this._accounts_service_access.notify["last-running-player"].connect(() => {
			this.menus.@foreach ( (profile, menu) => {
				menu.set_default_player (this._accounts_service_access.last_running_player);
			});
		});

		this.sync_preferred_players ();
		this.settings.changed["interested-media-players"].connect ( () => {
			this.sync_preferred_players ();
		});

		/* Hide the notification when the menu is shown */
		var shown_action = actions.lookup_action ("indicator-shown") as SimpleAction;
		shown_action.change_state.connect ((state) => {
			block_info_notifications = state.get_boolean();
			if (block_info_notifications) {
				debug("Indicator is shown");
				_info_notification.close();
			} else {
				debug("Indicator is hidden");
			}
		});

		/* Everything is built, let's put it on the bus */
		try {
			export_actions = bus.export_action_group ("/com/canonical/indicator/sound", this.actions);
		} catch (Error e) {
			critical ("%s", e.message);
		}

		this.menus.@foreach ( (profile, menu) => menu.export (bus, @"/com/canonical/indicator/sound/$profile"));
	}

	~Service() {
		debug("Destroying Service Object");

		clear_acts_player();

		if (this.player_action_update_id > 0) {
			Source.remove (this.player_action_update_id);
			this.player_action_update_id = 0;
		}

		if (this.sound_was_blocked_timeout_id > 0) {
			Source.remove (this.sound_was_blocked_timeout_id);
			this.sound_was_blocked_timeout_id = 0;
		}

		if (this.export_actions != 0) {
			bus.unexport_action_group(this.export_actions);
			this.export_actions = 0;
		}
	}

	bool greeter_show_track () {
		return export_to_accounts_service;
	}

	void clear_acts_player () {
		/* NOTE: This is a bit of a hack to ensure that accounts service doesn't
		   continue to export the player by keeping a ref in the timer */
		if (this.accounts_service != null)
			this.accounts_service.player = null;
	}

	public bool visible { get; set; }

	const ActionEntry[] action_entries = {
		{ "root", null, null, "@a{sv} {}", null },
		{ "scroll", activate_scroll_action, "i", null, null },
		{ "desktop-settings", activate_desktop_settings, null, null, null },
		{ "phone-settings", activate_phone_settings, null, null, null },
		{ "indicator-shown", null, null, "@b false", null },
	};

	SimpleActionGroup actions;
	HashTable<string, SoundMenu> menus;
	Settings settings;
	VolumeControl volume_control;
	MediaPlayerList players;
	uint player_action_update_id;
	bool mute_blocks_sound;
	uint sound_was_blocked_timeout_id;
	bool syncing_preferred_players = false;
	AccountsServiceUser? accounts_service = null;
	bool export_to_accounts_service = false;
	private Options _options;
	private VolumeWarning _volume_warning;
	private IndicatorSound.InfoNotification _info_notification = new IndicatorSound.InfoNotification();
	private AccountsServiceAccess _accounts_service_access;

	const double volume_step_percentage = 0.06;

	private void activate_scroll_action (SimpleAction action, Variant? param) {
		int direction = param.get_int32(); // positive for up, negative for down
		message("scroll: %d", direction);

		if (_volume_warning.active) {
			_volume_warning.user_keypress(direction>0
				? VolumeWarning.Key.VOLUME_UP
				: VolumeWarning.Key.VOLUME_DOWN);
		} else {
			double delta = volume_step_percentage * direction;
			double v = volume_control.volume.volume + delta;
			volume_control.set_volume_clamp (v, VolumeControl.VolumeReasons.USER_KEYPRESS);
		}
	}

	void activate_desktop_settings (SimpleAction action, Variant? param) {
		unowned string env = Environment.get_variable ("DESKTOP_SESSION");
		string cmd;

		if (Environment.get_variable ("MIR_SOCKET") != null)
		{
			UrlDispatch.send ("settings:///system/sound");
			return;
		}

		if (env == "xubuntu" || env == "ubuntustudio")
			cmd = "pavucontrol";
		else if (env == "mate")
			cmd = "mate-volume-control";
		else
		{
			if (Environment.get_variable ("XDG_CURRENT_DESKTOP") == "Unity" && Environment.find_program_in_path ("unity-control-center") != null)
				cmd = "unity-control-center sound";
			else
				cmd = "gnome-control-center sound";
		}

		try {
			Process.spawn_command_line_async (cmd);
		} catch (Error e) {
			warning ("unable to launch sound settings: %s", e.message);
		}
	}

	void activate_phone_settings (SimpleAction action, Variant? param) {
		UrlDispatch.send ("settings:///system/sound");
	}

	/* Returns a serialized version of @icon_name suited for the panel */
	static Variant serialize_themed_icon (string icon_name)
	{
		var icon = new ThemedIcon.with_default_fallbacks (icon_name);
		return icon.serialize ();
	}

	void update_root_icon () {
		double volume = this.volume_control.volume.volume;
		unowned string icon = get_volume_root_icon (volume, this.volume_control.mute, volume_control.active_output());

		string accessible_name;
		if (this.volume_control.mute) {
			accessible_name = _("Volume (muted)");
		} else if (this.accounts_service != null && this.accounts_service.silentMode) {
			int volume_int = (int)(volume * 100);
			accessible_name = "%s (%s %d%%)".printf (_("Volume"), _("silent"), volume_int);
		} else {
			int volume_int = (int)(volume * 100);
			accessible_name = "%s (%d%%)".printf (_("Volume"), volume_int);
		}

		var root_action = actions.lookup_action ("root") as SimpleAction;
		var builder = new VariantBuilder (VariantType.VARDICT);
		builder.add ("{sv}", "title", new Variant.string (_("Sound")));
		builder.add ("{sv}", "accessible-desc", new Variant.string (accessible_name));
		builder.add ("{sv}", "icon", serialize_themed_icon (icon));
		builder.add ("{sv}", "visible", new Variant.boolean (this.visible));
		root_action.set_state (builder.end());
	}

	private bool block_info_notifications = false;

	private static unowned string get_volume_root_icon_by_volume (double volume, VolumeControl.ActiveOutput active_output) {
		switch (active_output) {
			case VolumeControl.ActiveOutput.SPEAKERS:
			case VolumeControl.ActiveOutput.HEADPHONES:
			case VolumeControl.ActiveOutput.BLUETOOTH_HEADPHONES:
			case VolumeControl.ActiveOutput.BLUETOOTH_SPEAKER:
			case VolumeControl.ActiveOutput.USB_SPEAKER:
			case VolumeControl.ActiveOutput.USB_HEADPHONES:
			case VolumeControl.ActiveOutput.HDMI_SPEAKER:
			case VolumeControl.ActiveOutput.HDMI_HEADPHONES:
				if (volume <= 0.0)
					return "audio-volume-muted-panel";
				if (volume <= 0.3)
					return "audio-volume-low-panel";
				if (volume <= 0.7)
					return "audio-volume-medium-panel";
				return "audio-volume-high-panel";

			default:
				return "";
		}
	}

	private unowned string get_volume_root_icon (double volume, bool mute, VolumeControl.ActiveOutput active_output) {
		switch (active_output) {
			case VolumeControl.ActiveOutput.SPEAKERS:
			case VolumeControl.ActiveOutput.HEADPHONES:
			case VolumeControl.ActiveOutput.BLUETOOTH_HEADPHONES:
			case VolumeControl.ActiveOutput.BLUETOOTH_SPEAKER:
			case VolumeControl.ActiveOutput.USB_SPEAKER:
			case VolumeControl.ActiveOutput.USB_HEADPHONES:
			case VolumeControl.ActiveOutput.HDMI_SPEAKER:
			case VolumeControl.ActiveOutput.HDMI_HEADPHONES:
				if (mute || volume <= 0.0)
					return this.mute_blocks_sound ? "audio-volume-muted-blocking-panel" : "audio-volume-muted-panel";
				if (this.accounts_service != null && this.accounts_service.silentMode)
					return "audio-volume-muted-panel";
				return get_volume_root_icon_by_volume (volume, active_output);

			default:
				return "";
		}
	}

	private void update_notification () {
		if (!_volume_warning.active && !block_info_notifications) {
			_info_notification.show(this.volume_control.active_output(),
						get_volume_percent(),
			                        _volume_warning.high_volume);
		}
	}

	SimpleAction silent_action;
	Action create_silent_mode_action () {
		bool silentNow = false;
		if (this.accounts_service != null) {
			silentNow = this.accounts_service.silentMode;
		}

		silent_action = new SimpleAction.stateful ("silent-mode", null, new Variant.boolean (silentNow));

		/* If we're not dealing with accounts service, we'll just always be out
		   of silent mode and that's cool. */
		if (this.accounts_service == null) {
			return silent_action;
		}

		this.accounts_service.notify["silentMode"].connect(() => {
			silent_action.set_state(new Variant.boolean(this.accounts_service.silentMode));
			this.update_root_icon ();
		});

		silent_action.activate.connect ((action, param) => {
			action.change_state (new Variant.boolean (!action.get_state().get_boolean()));
		});

		silent_action.change_state.connect ((action, val) => {
			this.accounts_service.silentMode = val.get_boolean();
		});

		return silent_action;
	}

	SimpleAction mute_action;
	Action create_mute_action () {
		mute_action = new SimpleAction.stateful ("mute", null, new Variant.boolean (this.volume_control.mute));

		mute_action.activate.connect ( (action, param) => {
			action.change_state (new Variant.boolean (!action.get_state ().get_boolean ()));
		});

		mute_action.change_state.connect ( (action, val) => {
			volume_control.set_mute (val.get_boolean ());
		});

		this.volume_control.notify["mute"].connect ( () => {
			mute_action.set_state (new Variant.boolean (this.volume_control.mute));
			this.update_root_icon ();
		});

		this.volume_control.notify["is-playing"].connect( () => {
			if (!this.volume_control.mute) {
				this.mute_blocks_sound = false;
				return;
			}

			if (this.volume_control.is_playing) {
				this.mute_blocks_sound = true;
			}
			else if (this.mute_blocks_sound) {
				/* Continue to show the blocking icon five seconds after a player has tried to play something */
				if (this.sound_was_blocked_timeout_id > 0)
					Source.remove (this.sound_was_blocked_timeout_id);

				this.sound_was_blocked_timeout_id = Timeout.add_seconds (5, () => {
					this.mute_blocks_sound = false;
					this.sound_was_blocked_timeout_id = 0;
					this.update_root_icon ();
					return Source.REMOVE;
				});
			}

			this.update_root_icon ();
		});

		return mute_action;
	}

	/* return the current volume in the range of [0.0, 1.0] */
	private double get_volume_percent() {
		return volume_control.volume.volume / _options.max_volume;
	}

	/* volume control's range can vary depending on options.max_volume,
	 * but the action always needs to be in [0.0, 1.0]... */
	private Variant create_volume_action_state() {
		return new Variant.double (get_volume_percent());
	}

	private void update_volume_action_state() {
		volume_action.set_state(create_volume_action_state());
	}

	private SimpleAction volume_action;
	private Action create_volume_action () {
		volume_action = new SimpleAction.stateful ("volume", VariantType.INT32, create_volume_action_state());

		volume_action.change_state.connect ( (action, val) => {
			double v = val.get_double () * _options.max_volume;
			volume_control.set_volume_clamp (v, VolumeControl.VolumeReasons.USER_KEYPRESS);
		});

		/* activating this action changes the volume by the amount given in the parameter */
		volume_action.activate.connect ((a,p) => activate_scroll_action(a,p));

		_options.notify["max-volume"].connect(() => {
			update_volume_action_state();
		});

		this.volume_control.notify["volume"].connect (() => {
			update_volume_action_state();

			this.update_root_icon ();

			var reason = volume_control.volume.reason;
			if (reason == VolumeControl.VolumeReasons.USER_KEYPRESS ||
					reason == VolumeControl.VolumeReasons.DEVICE_OUTPUT_CHANGE)
				this.update_notification ();
		});

		this.volume_control.bind_property ("ready", volume_action, "enabled", BindingFlags.SYNC_CREATE);

		return volume_action;
	}

	SimpleAction mic_volume_action;
	Action create_mic_volume_action () {
		mic_volume_action = new SimpleAction.stateful ("mic-volume", null, new Variant.double (this.volume_control.mic_volume));

		mic_volume_action.change_state.connect ( (action, val) => {
			volume_control.mic_volume = val.get_double ();
		});

		this.volume_control.notify["mic-volume"].connect ( () => {
			mic_volume_action.set_state (new Variant.double (this.volume_control.mic_volume));
		});

		this.volume_control.bind_property ("ready", mic_volume_action, "enabled", BindingFlags.SYNC_CREATE);

		return mic_volume_action;
	}

	private Variant create_high_volume_action_state() {
		return new Variant.boolean (_volume_warning.high_volume);
	}
	private void update_high_volume_action_state() {
		high_volume_action.set_state(create_high_volume_action_state());
	}

	SimpleAction high_volume_action;
	Action create_high_volume_action () {
		high_volume_action = new SimpleAction.stateful("high-volume", null, create_high_volume_action_state());

		_volume_warning.notify["high-volume"].connect( () => {
			update_high_volume_action_state();
			update_notification();
		});

		return high_volume_action;
	}

	SimpleAction volume_sync_action;
	uint64 volume_sync_number_ = 0;
	Action create_volume_sync_action () {
		volume_sync_action = new SimpleAction.stateful("volume-sync", VariantType.UINT64, new Variant.uint64 (volume_sync_number_));

		return volume_sync_action;
	}

	void increment_volume_sync_action () {
		volume_sync_action.set_state(new Variant.uint64 (++volume_sync_number_));
	}

	uint export_actions = 0;

	Variant action_state_for_player (MediaPlayer player, bool show_track = true) {
		var builder = new VariantBuilder (VariantType.VARDICT);
		builder.add ("{sv}", "running", new Variant ("b", player.is_running));
		builder.add ("{sv}", "state", new Variant ("s", player.state));
		if (player.current_track != null && show_track) {
			builder.add ("{sv}", "title", new Variant ("s", player.current_track.title));
			builder.add ("{sv}", "artist", new Variant ("s", player.current_track.artist));
			builder.add ("{sv}", "album", new Variant ("s", player.current_track.album));
			builder.add ("{sv}", "art-url", new Variant ("s", player.current_track.art_url));
		}
		return builder.end ();
	}

	bool update_player_actions () {
		bool clear_accounts_player = true;

		foreach (var player in this.players) {
			SimpleAction? action = this.actions.lookup_action (player.id) as SimpleAction;
			if (action != null) {
				action.set_state (this.action_state_for_player (player));
				action.set_enabled (player.can_raise);
			}

			SimpleAction? greeter_action = this.actions.lookup_action (player.id + ".greeter") as SimpleAction;
			if (greeter_action != null) {
				greeter_action.set_state (this.action_state_for_player (player, greeter_show_track()));
				greeter_action.set_enabled (player.can_raise);
			}

			/* If we're playing then put that data in accounts service */
			if (player.is_running && export_to_accounts_service && accounts_service != null) {
				accounts_service.player = player;
				clear_accounts_player = false;
			}
		}

		if (clear_accounts_player)
			clear_acts_player();

		this.player_action_update_id = 0;
		return Source.REMOVE;
	}

	void eventually_update_player_actions () {
		if (player_action_update_id == 0)
			this.player_action_update_id = Idle.add (this.update_player_actions);
	}


	void sync_preferred_players () {
		this.syncing_preferred_players = true;
		this.players.sync (settings.get_strv ("interested-media-players"));
		this.syncing_preferred_players = false;
	}

	void update_preferred_players () {
		/* only write the key if we're not getting this call because we're syncing from the key right now */
		if (!this.syncing_preferred_players) {
			var builder = new VariantBuilder (VariantType.STRING_ARRAY);
			foreach (var player in this.players)
				builder.add ("s", player.id);
			this.settings.set_value ("interested-media-players", builder.end ());
		}
	}

	void player_added (MediaPlayer player) {
		this.menus.@foreach ( (profile, menu) => menu.add_player (player));

		SimpleAction action = new SimpleAction.stateful (player.id, null, this.action_state_for_player (player));
		action.set_enabled (player.can_raise);
		action.activate.connect ( () => { player.activate (); });
		this.actions.add_action (action);

		SimpleAction greeter_action = new SimpleAction.stateful (player.id + ".greeter", null, this.action_state_for_player (player, greeter_show_track()));
		greeter_action.set_enabled (player.can_raise);
		greeter_action.activate.connect ( () => { player.activate (); });
		this.actions.add_action (greeter_action);

		var play_action = new SimpleAction.stateful ("play." + player.id, null, player.state);
		play_action.activate.connect ( () => player.play_pause () );
		this.actions.add_action (play_action);
		player.notify["state"].connect ( (object, pspec) => {
			play_action.set_state (player.state);
		});

		var next_action = new SimpleAction ("next." + player.id, null);
		next_action.activate.connect ( () => player.next () );
		this.actions.add_action (next_action);

		var prev_action = new SimpleAction ("previous." + player.id, null);
		prev_action.activate.connect ( () => player.previous () );
		this.actions.add_action (prev_action);

		var playlist_action = new SimpleAction ("play-playlist." + player.id, VariantType.STRING);
		playlist_action.activate.connect ( (parameter) => player.activate_playlist_by_name (parameter.get_string ()) );
		this.actions.add_action (playlist_action);

		player.notify.connect (this.eventually_update_player_actions);

		this.update_preferred_players ();
	}

	void player_removed (MediaPlayer player) {
		this.actions.remove_action (player.id);
		this.actions.remove_action ("play." + player.id);
		this.actions.remove_action ("next." + player.id);
		this.actions.remove_action ("previous." + player.id);
		this.actions.remove_action ("play-playlist." + player.id);

		player.notify.disconnect (this.eventually_update_player_actions);

		this.menus.@foreach ( (profile, menu) => menu.remove_player (player));

		this.update_preferred_players ();
	}
}
