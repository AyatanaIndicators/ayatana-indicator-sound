
public class IndicatorSound.Service {
	public Service () {
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
		{ "mute", activate_mute, null, "false", null },
		{ "volume", null, null, "0", volume_changed },
		{ "settings", activate_settings, null, null, null }
	};

	MainLoop loop;
	SimpleActionGroup actions;
	Menu menu;

	void activate_mute (SimpleAction action, Variant? param) {
		bool muted = action.get_state ().get_boolean ();
	}

	void volume_changed (SimpleAction action, Variant val) {
	}

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

	void bus_acquired (DBusConnection connection, string name) {
		this.actions = new SimpleActionGroup ();
		this.actions.add_entries (action_entries, this);

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
