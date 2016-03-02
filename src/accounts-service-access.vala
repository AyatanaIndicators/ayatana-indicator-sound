/*
 * -*- Mode:Vala; indent-tabs-mode:t; tab-width:4; encoding:utf8 -*-
 * Copyright 2016 Canonical Ltd.
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
 *      Xavi Garcia <xavi.garcia.mena@canonical.com>
 */

using PulseAudio;
using Notify;
using Gee;

[DBus (name="com.canonical.UnityGreeter.List")]
interface GreeterListInterfaceAccess : Object
{
	public abstract async string get_active_entry () throws IOError;
	public signal void entry_selected (string entry_name);
}

public class AccountsServiceAccess : Object
{
	private DBusProxy _user_proxy;
	private GreeterListInterfaceAccess _greeter_proxy;
	private double _volume = 0.0;
	private string _last_running_player = "";
	private bool _mute = false;

	public AccountsServiceAccess ()
	{
		setup_accountsservice.begin ();
	}

	~AccountsServiceAccess ()
	{
	}

	public string last_running_player 
	{ 
		get 
		{ 
			return _last_running_player; 
		} 
		set 
		{ 
			sync_last_running_player_to_accountsservice.begin (value);
		} 
	}

	public bool mute 
	{ 
		get 
		{ 
			return _mute; 
		} 
		set 
		{ 
			sync_mute_to_accountsservice.begin (value);
		} 
	}

	public double volume 
	{ 
		get 
		{ 
			return _volume; 
		} 
		set 
		{ 
			sync_volume_to_accountsservice.begin (value);
		} 
	}

	/* AccountsService operations */
	private void accountsservice_props_changed_cb (DBusProxy proxy, Variant changed_properties, string[]? invalidated_properties)
	{
		Variant volume_variant = changed_properties.lookup_value ("Volume", VariantType.DOUBLE);
		if (volume_variant != null) {
			var volume = volume_variant.get_double ();
			if (volume >= 0 && _volume != volume) {
				_volume = volume;
				this.notify_property("volume");
			}
		}

		Variant mute_variant = changed_properties.lookup_value ("Muted", VariantType.BOOLEAN);
		if (mute_variant != null) {
			_mute = mute_variant.get_boolean ();
			this.notify_property("mute");
		}

		Variant last_running_player_variant = changed_properties.lookup_value ("LastRunningPlayer", VariantType.STRING);
		if (last_running_player_variant != null) {
			_last_running_player = last_running_player_variant.get_string ();
			this.notify_property("last-running-player");
		}
	}

	private async void setup_user_proxy (string? username_in = null)
	{
		var username = username_in;
		_user_proxy = null;

		// Look up currently selected greeter user, if asked
		if (username == null) {
			try {
				username = yield _greeter_proxy.get_active_entry ();
				if (username == "" || username == null)
					return;
			} catch (GLib.Error e) {
				warning ("unable to find Accounts path for user %s: %s", username == null ? "null" : username, e.message);
				return;
			}
		}

		// Get master AccountsService object
		DBusProxy accounts_proxy;
		try {
			accounts_proxy = yield DBusProxy.create_for_bus (BusType.SYSTEM, DBusProxyFlags.DO_NOT_LOAD_PROPERTIES | DBusProxyFlags.DO_NOT_CONNECT_SIGNALS, null, "org.freedesktop.Accounts", "/org/freedesktop/Accounts", "org.freedesktop.Accounts");
		} catch (GLib.Error e) {
			warning ("unable to get greeter proxy: %s", e.message);
			return;
		}

		// Find user's AccountsService object
		try {
			var user_path_variant = yield accounts_proxy.call ("FindUserByName", new Variant ("(s)", username), DBusCallFlags.NONE, -1);
			string user_path;
			if (user_path_variant.check_format_string ("(o)", true)) {
				user_path_variant.get ("(o)", out user_path);
				_user_proxy = yield DBusProxy.create_for_bus (BusType.SYSTEM, DBusProxyFlags.GET_INVALIDATED_PROPERTIES, null, "org.freedesktop.Accounts", user_path, "com.ubuntu.AccountsService.Sound");
			} else {
				warning ("Unable to find user name after calling FindUserByName. Expected type: %s and obtained %s", "(o)", user_path_variant.get_type_string () );
				return;
			}
		} catch (GLib.Error e) {
			warning ("unable to find Accounts path for user %s: %s", username, e.message);
			return;
		}

		// Get current values and listen for changes
		_user_proxy.g_properties_changed.connect (accountsservice_props_changed_cb);
		try {
			var props_variant = yield _user_proxy.get_connection ().call (_user_proxy.get_name (), _user_proxy.get_object_path (), "org.freedesktop.DBus.Properties", "GetAll", new Variant ("(s)", _user_proxy.get_interface_name ()), null, DBusCallFlags.NONE, -1);
			if (props_variant.check_format_string ("(@a{sv})", true)) {
				Variant props;
				props_variant.get ("(@a{sv})", out props);
				accountsservice_props_changed_cb(_user_proxy, props, null);
			} else {
				warning ("Unable to get accounts service properties after calling GetAll. Expected type: %s and obtained %s", "(@a{sv})", props_variant.get_type_string () );
				return;
			}
		} catch (GLib.Error e) {
			debug("Unable to get properties for user %s at first try: %s", username, e.message);
		}
	}	

	private void greeter_user_changed (string username)
	{
		setup_user_proxy.begin (username);
	}

	private async void setup_accountsservice ()
	{
		if (Environment.get_variable ("XDG_SESSION_CLASS") == "greeter") {
			try {
				_greeter_proxy = yield Bus.get_proxy (BusType.SESSION, "com.canonical.UnityGreeter", "/list");
			} catch (GLib.Error e) {
				warning ("unable to get greeter proxy: %s", e.message);
				return;
			}
			_greeter_proxy.entry_selected.connect (greeter_user_changed);
			yield setup_user_proxy ();
		} else {
			// We are in a user session.  We just need our own proxy
			unowned string username = Environment.get_variable ("USER");
			if (username != null && username != "") {
				yield setup_user_proxy (username);
			}
		}
	}

	private async void sync_last_running_player_to_accountsservice (string last_running_player)
	{
		if (_user_proxy == null)
			return;

		try {
			yield _user_proxy.get_connection ().call (_user_proxy.get_name (), _user_proxy.get_object_path (), "org.freedesktop.DBus.Properties", "Set", new Variant ("(ssv)", _user_proxy.get_interface_name (), "LastRunningPlayer", new Variant ("s", last_running_player)), null, DBusCallFlags.NONE, -1);
		} catch (GLib.Error e) {
			warning ("unable to sync last running player %s to AccountsService: %s",last_running_player, e.message);
		}
		_last_running_player = last_running_player;
	}

	private async void sync_volume_to_accountsservice (double volume)
	{
		if (_user_proxy == null)
			return;

		try {
			yield _user_proxy.get_connection ().call (_user_proxy.get_name (), _user_proxy.get_object_path (), "org.freedesktop.DBus.Properties", "Set", new Variant ("(ssv)", _user_proxy.get_interface_name (), "Volume", new Variant ("d", volume)), null, DBusCallFlags.NONE, -1);
		} catch (GLib.Error e) {
			warning ("unable to sync volume %f to AccountsService: %s", volume, e.message);
		}
	}

	private async void sync_mute_to_accountsservice (bool mute)
	{
		if (_user_proxy == null)
			return;

		try {
			yield _user_proxy.get_connection ().call (_user_proxy.get_name (), _user_proxy.get_object_path (), "org.freedesktop.DBus.Properties", "Set", new Variant ("(ssv)", _user_proxy.get_interface_name (), "Muted", new Variant ("b", mute)), null, DBusCallFlags.NONE, -1);
		} catch (GLib.Error e) {
			warning ("unable to sync mute %s to AccountsService: %s", mute ? "true" : "false", e.message);
		}
	}
}


