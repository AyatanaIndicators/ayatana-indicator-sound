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

public class AccountsServiceUser : Object {
	Act.UserManager accounts_manager = Act.UserManager.get_default();
	Act.User? user = null;
	AccountsServiceSoundSettings? proxy = null;
	AccountsServicePrivacySettings? privacyproxy = null;
	uint timer = 0;
	MediaPlayer? _player = null;
	GreeterBroadcast? greeter = null;

	public bool showDataOnGreeter { get; set; }

	public MediaPlayer? player {
		set {
			this._player = value;
			debug("New player: %s", this._player != null ? this._player.name : "Cleared");

			/* No proxy, no settings to set */
			if (this.proxy == null) {
				debug("Nothing written to Accounts Service, waiting on proxy");
				return;
			}

			/* Always reset the timer */
			if (this.timer != 0) {
				GLib.Source.remove(this.timer);
				this.timer = 0;
			}

			if (this._player == null) {
				debug("Clearing player data in accounts service");

				/* Clear it */
				this.proxy.player_name = "";
				this.proxy.timestamp = 0;
				this.proxy.title = "";
				this.proxy.artist = "";
				this.proxy.album = "";
				this.proxy.art_url = "";

				var icon = new ThemedIcon.with_default_fallbacks ("application-default-icon");
				this.proxy.player_icon = icon.serialize();
			} else {
				this.proxy.timestamp = GLib.get_monotonic_time();
				this.proxy.player_name = this._player.name;

				/* Serialize the icon if it exits, if it doesn't or errors then
				   we need to use the application default icon */
				GLib.Variant? icon_serialization = null;
				if (this._player.icon != null)
					icon_serialization = this._player.icon.serialize();
				if (icon_serialization == null) {
					var icon = new ThemedIcon.with_default_fallbacks ("application-default-icon");
					icon_serialization = icon.serialize();
				}
				this.proxy.player_icon = icon_serialization;

				/* Set state of the player */
				this.proxy.running = this._player.is_running;
				this.proxy.state = this._player.state;

				if (this._player.current_track != null) {
					this.proxy.title = this._player.current_track.title;
					this.proxy.artist = this._player.current_track.artist;
					this.proxy.album = this._player.current_track.album;
					this.proxy.art_url = this._player.current_track.art_url;
				} else {
					this.proxy.title = "";
					this.proxy.artist = "";
					this.proxy.album = "";
					this.proxy.art_url = "";
				}

				this.timer = GLib.Timeout.add_seconds(5 * 60, () => {
					debug("Writing timestamp");
					this.proxy.timestamp = GLib.get_monotonic_time();
					return true;
				});
			}
		}
		get {
			return this._player;
		}
	}

	public AccountsServiceUser () {
		user = accounts_manager.get_user(GLib.Environment.get_user_name());
		user.notify["is-loaded"].connect(() => user_loaded_changed());
		user_loaded_changed();

		Bus.get_proxy.begin<GreeterBroadcast> (
			BusType.SYSTEM,
			"com.canonical.Unity.Greeter.Broadcast",
			"/com/canonical/Unity/Greeter/Broadcast",
			DBusProxyFlags.NONE,
			null,
			greeter_proxy_new);
	}

	void user_loaded_changed () {
		debug("User loaded changed");

		this.proxy = null;

		if (this.user.is_loaded) {
			Bus.get_proxy.begin<AccountsServiceSoundSettings> (
				BusType.SYSTEM,
				"org.freedesktop.Accounts",
				user.get_object_path(),
				DBusProxyFlags.GET_INVALIDATED_PROPERTIES,
				null,
				new_sound_proxy);

			Bus.get_proxy.begin<AccountsServicePrivacySettings> (
				BusType.SYSTEM,
				"org.freedesktop.Accounts",
				user.get_object_path(),
				DBusProxyFlags.GET_INVALIDATED_PROPERTIES,
				null,
				new_privacy_proxy);
		}
	}

	~AccountsServiceUser () {
		debug("Account Service Object Finalizing");
		this.player = null;

		if (this.timer != 0) {
			GLib.Source.remove(this.timer);
			this.timer = 0;
		}
	}

	void new_sound_proxy (GLib.Object? obj, AsyncResult res) {
		try {
			this.proxy = Bus.get_proxy.end (res);
			this.player = _player;
		} catch (Error e) {
			this.proxy = null;
			warning("Unable to get proxy to user sound settings: %s", e.message);
		}
	}

	void new_privacy_proxy (GLib.Object? obj, AsyncResult res) {
		try {
			this.privacyproxy = Bus.get_proxy.end (res);

			(this.privacyproxy as DBusProxy).g_properties_changed.connect((proxy, changed, invalid) => {
				var welcomeval = changed.lookup_value("MessagesWelcomeScreen", new VariantType("b"));
				if (welcomeval != null) {
					debug("Messages on welcome screen changed");
					this.showDataOnGreeter = welcomeval.get_boolean();
				}
			});

			this.showDataOnGreeter = this.privacyproxy.messages_welcome_screen;
		} catch (Error e) {
			this.privacyproxy = null;
			warning("Unable to get proxy to user privacy settings: %s", e.message);
		}
	}

	void greeter_proxy_new (GLib.Object? obj, AsyncResult res) {
		try {
			this.greeter = Bus.get_proxy.end (res);

			this.greeter.SoundPlayPause.connect((username) => {
				if (username != GLib.Environment.get_user_name())
					return;
				if (this._player == null)
					return;
				this._player.play_pause();
			});

			this.greeter.SoundNext.connect((username) => {
				if (username != GLib.Environment.get_user_name())
					return;
				if (this._player == null)
					return;
				this._player.next();
			});

			this.greeter.SoundPrev.connect((username) => {
				if (username != GLib.Environment.get_user_name())
					return;
				if (this._player == null)
					return;
				this._player.previous();
			});
		} catch (Error e) {
			this.greeter = null;
			warning("Unable to get greeter proxy: %s", e.message);
		}
	}
}
