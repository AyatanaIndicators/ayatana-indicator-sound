
public class IndicatorSound.Service {
	public Service () {
		this.volume_control = new VolumeControl ();

		this.players = new MediaPlayerList ();
		this.players.player_added.connect (player_added);
	}

	public int run () {
		if (this.loop != null) {
			warning ("service is already running");
			return 1;
		}

		Bus.own_name (BusType.SESSION, "com.canonical.indicator.sound", BusNameOwnerFlags.NONE,
			this.bus_acquired, null, this.name_lost);

		this.loop = new MainLoop (null, false);
		this.loop.run ();

		return 0;
	}

	const ActionEntry[] action_entries = {
		{ "root", null, null, "('', 'audio-volume-high-panel', '', true)", null },
		{ "settings", activate_settings, null, null, null }
	};

	MainLoop loop;
	SimpleActionGroup actions;
	Menu menu;
	VolumeControl volume_control;
	MediaPlayerList players;
	uint player_action_update_id;

	void activate_settings (SimpleAction action, Variant? param) {
		try {
			Process.spawn_command_line_async ("gnome-control-center sound");
		} catch (Error e) {
			warning ("unable to launch sound settings: %s", e.message);
		}
	}

	static Menu create_base_menu () {
		var submenu = new Menu ();
		submenu.append ("Mute", "indicator.mute");

		var slider = new MenuItem (null, "indicator.volume");
		slider.set_attribute ("x-canonical-type", "s", "com.canonical.unity.slider");
		submenu.append_item (slider);

		submenu.append ("Sound Settings…", "indicator.settings");

		var root = new MenuItem (null, "indicator.root");
		root.set_attribute ("x-canonical-type", "s", "com.canonical.indicator.root");
		root.set_submenu (submenu);

		var menu = new Menu ();
		menu.append_item (root);

		return menu;
	}

	Action create_mute_action () {
		var mute_action = new SimpleAction.stateful ("mute", null, this.volume_control.mute);

		mute_action.activate.connect ( (action, param) => {
			action.change_state (!action.get_state ().get_boolean ());
		});

		mute_action.change_state.connect ( (action, val) => {
			volume_control.set_mute (val.get_boolean ());
		});

		this.volume_control.notify["mute"].connect ( () => {
			mute_action.set_state (this.volume_control.mute);
		});

		return mute_action;
	}

	Action create_volume_action () {
		var volume_action = new SimpleAction.stateful ("volume", null, this.volume_control.get_volume ());

		volume_action.change_state.connect ( (action, val) => {
			volume_control.set_volume (val.get_double ());
		});

		this.volume_control.volume_changed.connect ( (volume) => {
			volume_action.set_state (volume);
		});

		this.volume_control.bind_property ("ready", volume_action, "enabled", BindingFlags.SYNC_CREATE);

		return volume_action;
	}

	void bus_acquired (DBusConnection connection, string name) {
		this.actions = new SimpleActionGroup ();
		this.actions.add_entries (action_entries, this);

		this.actions.add_action (this.create_mute_action ());
		this.actions.add_action (this.create_volume_action ());

		this.menu = create_base_menu ();

		try {
			connection.export_action_group ("/com/canonical/indicator/sound", this.actions);
			connection.export_menu_model ("/com/canonical/indicator/sound/desktop", this.menu);
		} catch (Error e) {
			critical ("%s", e.message);
		}
	}

	void name_lost (DBusConnection connection, string name) {
		this.loop.quit ();
	}

	bool update_player_action (MediaPlayer player) {
		var builder = new VariantBuilder (new VariantType ("a{sv}"));
		builder.add ("{sv}", "running", new Variant ("b", player.is_running));
		var state = builder.end ();

		SimpleAction? action = this.actions.lookup (player.id) as SimpleAction;
		if (action == null) {
			action = new SimpleAction.stateful (player.id, null, state);
			action.activate.connect ( () => { player.launch (); });
			this.actions.insert (action);
		}
		else {
			action.set_state (state);
		}

		this.player_action_update_id = 0;
		return false;
	}

	void eventually_update_player_action (MediaPlayer player) {
		if (player_action_update_id == 0)
			this.player_action_update_id = Idle.add ( () => this.update_player_action (player) );
	}

	void player_added (MediaPlayer player) {
		var item = new MenuItem (player.name, player.id);
		item.set_attribute ("x-canonical-type", "s", "com.canonical.unity.media-player");
		this.menu.insert_item (this.menu.get_n_items () -1, item);

		eventually_update_player_action (player);
		player.notify.connect ( () => eventually_update_player_action (player) );
	}
}
