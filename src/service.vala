
public class IndicatorSound.Service {
	public Service () {
		this.volume_control = new VolumeControl ();
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

		var slider = new MenuItem ("null", "indicator.volume");
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
}
