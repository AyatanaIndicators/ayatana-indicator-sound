/*
 * -*- Mode:Vala; indent-tabs-mode:t; tab-width:4; encoding:utf8 -*-
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
 *      Alberto Ruiz <alberto.ruiz@canonical.com>
 */

using PulseAudio;
using Notify;
using Gee;

[DBus (name="com.canonical.UnityGreeter.List")]
interface GreeterListInterface : Object
{
	public abstract async string get_active_entry () throws IOError;
	public signal void entry_selected (string entry_name);
}

public class VolumeControlPulse : VolumeControl
{
	private unowned PulseAudio.GLibMainLoop loop = null;

	private uint _reconnect_timer = 0;

	private PulseAudio.Context context;
	private bool   _mute = true;
	private VolumeControl.Volume _volume = new VolumeControl.Volume();
	private double _mic_volume = 0.0;

	/* Used by the pulseaudio stream restore extension */
	private DBusConnection _pconn;
	/* Need both the list and hash so we can retrieve the last known sink-input after
	 * releasing the current active one (restoring back to the previous known role) */
	private Gee.ArrayList<uint32> _sink_input_list = new Gee.ArrayList<uint32> ();
	private HashMap<uint32, string> _sink_input_hash = new HashMap<uint32, string> ();
	private bool _pulse_use_stream_restore = false;
	private int32 _active_sink_input = -1;
	private string[] _valid_roles = {"multimedia", "alert", "alarm", "phone"};
	private string? _objp_role_multimedia = null;
	private string? _objp_role_alert = null;
	private string? _objp_role_alarm = null;
	private string? _objp_role_phone = null;
	private uint _pa_volume_sig_count = 0;

	private DBusProxy _user_proxy;
	private GreeterListInterface _greeter_proxy;
	private Cancellable _mute_cancellable;
	private Cancellable _volume_cancellable;
	private uint _local_volume_timer = 0;
	private uint _accountservice_volume_timer = 0;
	private bool _send_next_local_volume = false;
	private double _account_service_volume = 0.0;
	private VolumeControl.ActiveOutput _active_output = VolumeControl.ActiveOutput.SPEAKERS;

	/** true when a microphone is active **/
	public override bool active_mic { get; private set; default = false; }

	public VolumeControlPulse (IndicatorSound.Options options, PulseAudio.GLibMainLoop loop)
	{
		base(options);

		_volume.volume = 0.0;
		_volume.reason = VolumeControl.VolumeReasons.PULSE_CHANGE;

		this.loop = loop;

		_mute_cancellable = new Cancellable ();
		_volume_cancellable = new Cancellable ();

		setup_accountsservice.begin ();

		this.reconnect_to_pulse ();
	}

	~VolumeControlPulse ()
	{
		stop_all_timers();
	}

	private void stop_all_timers()
	{
		if (_reconnect_timer != 0) {
			Source.remove (_reconnect_timer);
			_reconnect_timer = 0;
		}
		stop_local_volume_timer();
		stop_account_service_volume_timer();
	}

	public static VolumeControl.ActiveOutput calculate_active_output (SinkInfo? sink) {
		
		VolumeControl.ActiveOutput ret_output = VolumeControl.ActiveOutput.SPEAKERS;
		/* Check if the current active port is headset/headphone */
    		/* There is not easy way to check if the port is a headset/headphone besides
    		 * checking for the port name. On touch (with the pulseaudio droid element)
    		 * the headset/headphone port is called 'output-headset' and 'output-headphone'.
    		 * On the desktop this is usually called 'analog-output-headphones' */
		
		// first of all check if we are in call mode
		if (sink.active_port != null && sink.active_port.name == "output-speaker+wired_headphone") {
			return VolumeControl.ActiveOutput.CALL_MODE;
		}
		// look if it's a headset/headphones
		if (sink.name == "indicator_sound_test_headphones" ||
			(sink.active_port != null && 
			 (sink.active_port.name.contains("headset") ||
		          sink.active_port.name.contains("headphone")))) {
	    			// check if it's a bluetooth device
	    			unowned string device_bus = sink.proplist.gets ("device.bus");
	    			if (device_bus != null && device_bus == "bluetooth") {
					ret_output = VolumeControl.ActiveOutput.BLUETOOTH_HEADPHONES;
        			} else if (device_bus != null && device_bus == "usb") {
					ret_output = VolumeControl.ActiveOutput.USB_HEADPHONES;
				} else if (device_bus != null && device_bus == "hdmi") {
					ret_output = VolumeControl.ActiveOutput.HDMI_HEADPHONES;
				} else {
					ret_output = VolumeControl.ActiveOutput.HEADPHONES;
        		}
		} else {
			// speaker
			unowned string device_bus = sink.proplist.gets ("device.bus");
	    		if (device_bus != null && device_bus == "bluetooth") {
	    		    ret_output = VolumeControl.ActiveOutput.BLUETOOTH_SPEAKER;
        		} else if (device_bus != null && device_bus == "usb") {
				ret_output = VolumeControl.ActiveOutput.USB_SPEAKER;
			} else if (device_bus != null && device_bus == "hdmi") {
				ret_output = VolumeControl.ActiveOutput.HDMI_SPEAKER;
			} else {
				ret_output = VolumeControl.ActiveOutput.SPEAKERS;
        		}
		}

		return ret_output;
	}

	/* PulseAudio logic*/
	private void context_events_cb (Context c, Context.SubscriptionEventType t, uint32 index)
	{
		switch (t & Context.SubscriptionEventType.FACILITY_MASK)
		{
			case Context.SubscriptionEventType.SINK:
				update_sink ();
				break;

			case Context.SubscriptionEventType.SINK_INPUT:
				switch (t & Context.SubscriptionEventType.TYPE_MASK)
				{
					case Context.SubscriptionEventType.NEW:
						c.get_sink_input_info (index, handle_new_sink_input_cb);
						break;

					case Context.SubscriptionEventType.CHANGE:
						c.get_sink_input_info (index, handle_changed_sink_input_cb);
						break;

					case Context.SubscriptionEventType.REMOVE:
						remove_sink_input_from_list (index);
						break;
					default:
						debug ("Sink input event not known.");
						break;
				}
				break;

			case Context.SubscriptionEventType.SOURCE:
				update_source ();
				break;

			case Context.SubscriptionEventType.SOURCE_OUTPUT:
				switch (t & Context.SubscriptionEventType.TYPE_MASK)
				{
					case Context.SubscriptionEventType.NEW:
						c.get_source_output_info (index, source_output_info_cb);
						break;

					case Context.SubscriptionEventType.REMOVE:
						this.active_mic = false;
						break;
				}
				break;
		}
	}

	private void sink_info_cb_for_props (Context c, SinkInfo? i, int eol)
	{
		if (i == null)
			return;

		if (_mute != (bool)i.mute)
		{
			_mute = (bool)i.mute;
			this.notify_property ("mute");
		}

		var playing = (i.state == PulseAudio.SinkState.RUNNING);
		if (is_playing != playing)
			is_playing = playing;

		var oldval = _active_output;
		var newval = calculate_active_output(i);

		_active_output = newval;

		// Emit a change signal iff CALL_MODE wasn't involved. (FIXME: yuck.)
		if ((oldval != VolumeControl.ActiveOutput.CALL_MODE) &&
		    (newval != VolumeControl.ActiveOutput.CALL_MODE) &&
		    (oldval != newval)) {
			this.active_output_changed (newval);
		}

		if (_pulse_use_stream_restore == false &&
				_volume.volume != volume_to_double (i.volume.max ()))
		{
			var vol = new VolumeControl.Volume();
			vol.volume = volume_to_double (i.volume.max ());
			vol.reason = VolumeControl.VolumeReasons.PULSE_CHANGE;
			this.volume = vol;
		}
	}

	private void source_info_cb (Context c, SourceInfo? i, int eol)
	{
		if (i == null)
			return;

		if (_mic_volume != volume_to_double (i.volume.values[0]))
		{
			_mic_volume = volume_to_double (i.volume.values[0]);
			this.notify_property ("mic-volume");
		}
	}

	private void server_info_cb_for_props (Context c, ServerInfo? i)
	{
		if (i == null)
			return;
		context.get_sink_info_by_name (i.default_sink_name, sink_info_cb_for_props);
	}

	private void update_sink ()
	{
		context.get_server_info (server_info_cb_for_props);
	}

	private void update_source_get_server_info_cb (PulseAudio.Context c, PulseAudio.ServerInfo? i) {
		if (i != null)
			context.get_source_info_by_name (i.default_source_name, source_info_cb);
	}

	private void update_source ()
	{
		context.get_server_info (update_source_get_server_info_cb);
	}

	private DBusMessage pulse_dbus_filter (DBusConnection connection, owned DBusMessage message, bool incoming)
	{
		if (message.get_message_type () == DBusMessageType.SIGNAL) {
			string active_role_objp = _objp_role_alert;
			if (_active_sink_input != -1)
				active_role_objp = _sink_input_hash.get (_active_sink_input);

			if (message.get_path () == active_role_objp && message.get_member () == "VolumeUpdated") {
				uint sig_count = 0;
				lock (_pa_volume_sig_count) {
					sig_count = _pa_volume_sig_count;
					if (_pa_volume_sig_count > 0)
						_pa_volume_sig_count--;
				}

				/* We only care about signals if our internal count is zero */
				if (sig_count == 0) {
					/* Extract volume and make sure it's not a side effect of us setting it */
					Variant body = message.get_body ();
					Variant varray = body.get_child_value (0);

					uint32 type = 0, lvolume = 0;
					VariantIter iter = varray.iterator ();
					iter.next ("(uu)", &type, &lvolume);
					/* Here we need to compare integer values to avoid rounding issues, so just
					 * using the volume values used by pulseaudio */
					PulseAudio.Volume cvolume = double_to_volume (_volume.volume);
					if (lvolume != cvolume) {
						/* Someone else changed the volume for this role, reflect on the indicator */
						var vol = new VolumeControl.Volume();
						vol.volume = volume_to_double (lvolume);
						vol.reason = VolumeControl.VolumeReasons.PULSE_CHANGE;
						this.volume = vol;
					}
				}
			}
		}

		return message;
	}

	private VolumeControl.Stream calculate_active_stream()
	{
		if (_active_sink_input != -1) {
			var path = _sink_input_hash[_active_sink_input];
			if (path == _objp_role_multimedia)
				return Stream.MULTIMEDIA;
			if (path == _objp_role_alarm)
				return Stream.ALARM;
			if (path == _objp_role_phone)
				return Stream.PHONE;
		}

		return VolumeControl.Stream.ALERT;
	}

	private async void update_active_sink_input (int32 index)
	{
		if ((index == -1) || (index != _active_sink_input && index in _sink_input_list)) {
			string sink_input_objp = _objp_role_alert;
			if (index != -1)
				sink_input_objp = _sink_input_hash.get (index);
			_active_sink_input = index;
			var stream = calculate_active_stream();
			if (active_stream != stream) {
				active_stream = stream;
			}

			/* Listen for role volume changes from pulse itself (external clients) */
			try {
				var builder = new VariantBuilder (VariantType.OBJECT_PATH_ARRAY);
				builder.add ("o", sink_input_objp);

				yield _pconn.call ("org.PulseAudio.Core1", "/org/pulseaudio/core1",
						"org.PulseAudio.Core1", "ListenForSignal",
						new Variant ("(sao)", "org.PulseAudio.Ext.StreamRestore1.RestoreEntry.VolumeUpdated", builder),
						null, DBusCallFlags.NONE, -1);
			} catch (GLib.Error e) {
				warning ("unable to listen for pulseaudio dbus signals (%s)", e.message);
			}

			try {
				var props_variant = yield _pconn.call ("org.PulseAudio.Ext.StreamRestore1.RestoreEntry",
						sink_input_objp, "org.freedesktop.DBus.Properties", "Get",
						new Variant ("(ss)", "org.PulseAudio.Ext.StreamRestore1.RestoreEntry", "Volume"),
						null, DBusCallFlags.NONE, -1);
				Variant tmp;
				props_variant.get ("(v)", out tmp);
				uint32 type = 0, volume = 0;
				VariantIter iter = tmp.iterator ();
				iter.next ("(uu)", &type, &volume);

				var vol = new VolumeControl.Volume();
				vol.volume = volume_to_double (volume);
				vol.reason = VolumeControl.VolumeReasons.VOLUME_STREAM_CHANGE;
				this.volume = vol;
			} catch (GLib.Error e) {
				warning ("unable to get volume for active role %s (%s)", sink_input_objp, e.message);
			}
		}
	}

	private void add_sink_input_into_list (SinkInputInfo sink_input)
	{
		/* We're only adding ones that are not corked and with a valid role */
		unowned string role = sink_input.proplist.gets (PulseAudio.Proplist.PROP_MEDIA_ROLE);

		if (role != null && role in _valid_roles) {
			if (sink_input.corked == 0 || role == "phone") {
				_sink_input_list.insert (0, sink_input.index);
				switch (role)
				{
					case "multimedia":
						_sink_input_hash.set (sink_input.index, _objp_role_multimedia);
						break;
					case "alert":
						_sink_input_hash.set (sink_input.index, _objp_role_alert);
						break;
					case "alarm":
						_sink_input_hash.set (sink_input.index, _objp_role_alarm);
						break;
					case "phone":
						_sink_input_hash.set (sink_input.index, _objp_role_phone);
						break;
				}
				/* Only switch the active sink input in case a phone one is not active */
				if (_active_sink_input == -1 ||
						_sink_input_hash.get (_active_sink_input) != _objp_role_phone)
					update_active_sink_input.begin ((int32)sink_input.index);
			}
		}
	}

	private void remove_sink_input_from_list (uint32 index)
	{
		if (index in _sink_input_list) {
			_sink_input_list.remove (index);
			_sink_input_hash.unset (index);
			if (index == _active_sink_input) {
				if (_sink_input_list.size != 0)
					update_active_sink_input.begin ((int32)_sink_input_list.get (0));
				else
					update_active_sink_input.begin (-1);
			}
		}
	}

	private void handle_new_sink_input_cb (Context c, SinkInputInfo? i, int eol)
	{
		if (i == null)
			return;

		add_sink_input_into_list (i);
	}

	private void handle_changed_sink_input_cb (Context c, SinkInputInfo? i, int eol)
	{
		if (i == null)
			return;

		if (i.index in _sink_input_list) {
			/* Phone stream is always corked, so handle it differently */
			if (i.corked == 1 && _sink_input_hash.get (i.index) != _objp_role_phone)
				remove_sink_input_from_list (i.index);
		} else {
			if (i.corked == 0)
				add_sink_input_into_list (i);
		}
	}

	private void source_output_info_cb (Context c, SourceOutputInfo? i, int eol)
	{
		if (i == null)
			return;

		unowned string role = i.proplist.gets (PulseAudio.Proplist.PROP_MEDIA_ROLE);
		if (role == "phone" || role == "production")
			this.active_mic = true;
	}

	private void context_state_callback (Context c)
	{
		switch (c.get_state ()) {
			case Context.State.READY:
				if (_pulse_use_stream_restore) {
					c.subscribe (PulseAudio.Context.SubscriptionMask.SINK |
							PulseAudio.Context.SubscriptionMask.SINK_INPUT |
							PulseAudio.Context.SubscriptionMask.SOURCE |
							PulseAudio.Context.SubscriptionMask.SOURCE_OUTPUT);
				} else {
					c.subscribe (PulseAudio.Context.SubscriptionMask.SINK |
							PulseAudio.Context.SubscriptionMask.SOURCE |
							PulseAudio.Context.SubscriptionMask.SOURCE_OUTPUT);
				}
				c.set_subscribe_callback (context_events_cb);
				update_sink ();
				update_source ();
				this.ready = true; // true because we're connected to the pulse server
				break;

			case Context.State.FAILED:
			case Context.State.TERMINATED:
				if (_reconnect_timer == 0)
					_reconnect_timer = Timeout.add_seconds (2, reconnect_timeout);
				break;

			default:
				this.ready = false;
				break;
		}
	}

	bool reconnect_timeout ()
	{
		_reconnect_timer = 0;
		reconnect_to_pulse ();
		return Source.REMOVE;
	}

	void reconnect_to_pulse ()
	{
		if (this.ready) {
			this.context.disconnect ();
			this.context = null;
			this.ready = false;
		}

		var props = new Proplist ();
		props.sets (Proplist.PROP_APPLICATION_NAME, "Ubuntu Audio Settings");
		props.sets (Proplist.PROP_APPLICATION_ID, "com.canonical.settings.sound");
		props.sets (Proplist.PROP_APPLICATION_ICON_NAME, "multimedia-volume-control");
		props.sets (Proplist.PROP_APPLICATION_VERSION, "0.1");

		reconnect_pulse_dbus ();

		this.context = new PulseAudio.Context (loop.get_api(), null, props);
		this.context.set_state_callback (context_state_callback);

		unowned string server_string = Environment.get_variable("PULSE_SERVER");
		if (context.connect(server_string, Context.Flags.NOFAIL, null) < 0)
			warning( "pa_context_connect() failed: %s\n", PulseAudio.strerror(context.errno()));
	}

	void sink_info_list_callback_set_mute (PulseAudio.Context context, PulseAudio.SinkInfo? sink, int eol) {
		if (sink != null)
			context.set_sink_mute_by_index (sink.index, true, null);
	}

	void sink_info_list_callback_unset_mute (PulseAudio.Context context, PulseAudio.SinkInfo? sink, int eol) {
		if (sink != null)
			context.set_sink_mute_by_index (sink.index, false, null);
	}

	/* Mute operations */
	bool set_mute_internal (bool mute)
	{
		return_val_if_fail (context.get_state () == Context.State.READY, false);

		if (_mute != mute) {
			if (mute)
				context.get_sink_info_list (sink_info_list_callback_set_mute);
			else
				context.get_sink_info_list (sink_info_list_callback_unset_mute);
			return true;
		} else {
			return false;
		}
	}

	public override void set_mute (bool mute)
	{
		if (set_mute_internal (mute))
			sync_mute_to_accountsservice.begin (mute);
	}

	public void toggle_mute ()
	{
		this.set_mute (!this._mute);
	}

	public override bool mute
	{
		get
		{
			return this._mute;
		}
	}

	public override VolumeControl.ActiveOutput active_output()
	{
		return _active_output;
	}

	/* Volume operations */
	public static PulseAudio.Volume double_to_volume (double vol)
	{
		double tmp = (double)(PulseAudio.Volume.NORM - PulseAudio.Volume.MUTED) * vol;
		return (PulseAudio.Volume)tmp + PulseAudio.Volume.MUTED;
	}

	public static double volume_to_double (PulseAudio.Volume vol)
	{
		double tmp = (double)(vol - PulseAudio.Volume.MUTED);
		return tmp / (double)(PulseAudio.Volume.NORM - PulseAudio.Volume.MUTED);
	}

	private void set_volume_success_cb (Context c, int success)
	{
		if ((bool)success)
			this.notify_property("volume");
	}

	private void sink_info_set_volume_cb (Context c, SinkInfo? i, int eol)
	{
		if (i == null)
			return;

		unowned CVolume cvol = i.volume;
		cvol.scale (double_to_volume (_volume.volume));
		c.set_sink_volume_by_index (i.index, cvol, set_volume_success_cb);
	}

	private void server_info_cb_for_set_volume (Context c, ServerInfo? i)
	{
		if (i == null)
		{
			warning ("Could not get PulseAudio server info");
			return;
		}

		context.get_sink_info_by_name (i.default_sink_name, sink_info_set_volume_cb);
	}

	private async void set_volume_active_role ()
	{
		string active_role_objp = _objp_role_alert;

		if (_active_sink_input != -1 && _active_sink_input in _sink_input_list)
			active_role_objp = _sink_input_hash.get (_active_sink_input);

		try {
			double vol = _volume.volume;
			var builder = new VariantBuilder (new VariantType ("a(uu)"));
			builder.add ("(uu)", 0, double_to_volume (vol));
			Variant volume = builder.end ();

			/* Increase the signal counter so we can handle the callback */
			lock (_pa_volume_sig_count) {
				_pa_volume_sig_count++;
			}

			yield _pconn.call ("org.PulseAudio.Ext.StreamRestore1.RestoreEntry",
					active_role_objp, "org.freedesktop.DBus.Properties", "Set",
					new Variant ("(ssv)", "org.PulseAudio.Ext.StreamRestore1.RestoreEntry", "Volume", volume),
					null, DBusCallFlags.NONE, -1);
		} catch (GLib.Error e) {
			lock (_pa_volume_sig_count) {
				_pa_volume_sig_count--;
			}
			warning ("unable to set volume for stream obj path %s (%s)", active_role_objp, e.message);
		}
	}

	void set_mic_volume_success_cb (Context c, int success)
	{
		if ((bool)success)
			this.notify_property ("mic-volume");
	}

	void set_mic_volume_get_server_info_cb (PulseAudio.Context c, PulseAudio.ServerInfo? i) {
		if (i != null) {
			unowned CVolume cvol = CVolume ();
			cvol.set (1, double_to_volume (_mic_volume));
			c.set_source_volume_by_name (i.default_source_name, cvol, set_mic_volume_success_cb);
		}
	}

	public override VolumeControl.Volume volume {
		get {
			return _volume;
		}
		set {
			var volume_changed = (value.volume != _volume.volume);
			debug("Setting volume to %f for profile %d because %d", value.volume, _active_sink_input, value.reason);

			_volume = value;

			/* Make sure we're connected to Pulse and pulse didn't give us the change */
			if (context.get_state () == Context.State.READY &&
					_volume.reason != VolumeControl.VolumeReasons.PULSE_CHANGE &&
					volume_changed)
				if (_pulse_use_stream_restore)
					set_volume_active_role.begin ();
				else
					context.get_server_info (server_info_cb_for_set_volume);


			if (volume.reason != VolumeControl.VolumeReasons.ACCOUNTS_SERVICE_SET
				&& volume_changed) {
				start_local_volume_timer();
			}
		}
	}

	/** MIC VOLUME PROPERTY */

	public override double mic_volume {
		get {
			return _mic_volume;
		}
		set {
			return_if_fail (context.get_state () == Context.State.READY);

			_mic_volume = value;

			context.get_server_info (set_mic_volume_get_server_info_cb);
		}
	}

	public static DBusConnection? create_pulse_dbus_connection()
	{
		unowned string pulse_dbus_server_env = Environment.get_variable ("PULSE_DBUS_SERVER");
		string address;

		if (pulse_dbus_server_env != null) {
			address = pulse_dbus_server_env;
		} else {
			DBusConnection conn;
			Variant props;

			try {
				conn = Bus.get_sync (BusType.SESSION);
			} catch (GLib.IOError e) {
				warning ("unable to get the dbus session bus: %s", e.message);
				return null;
			}

			try {
				var props_variant = conn.call_sync ("org.PulseAudio1",
						"/org/pulseaudio/server_lookup1", "org.freedesktop.DBus.Properties",
						"Get", new Variant ("(ss)", "org.PulseAudio.ServerLookup1", "Address"),
						null, DBusCallFlags.NONE, -1);
				props_variant.get ("(v)", out props);
				address = props.get_string ();
			} catch (GLib.Error e) {
				warning ("unable to get pulse unix socket: %s", e.message);
				return null;
			}
		}

		DBusConnection conn = null;
		try {
			conn = new DBusConnection.for_address_sync (address, DBusConnectionFlags.AUTHENTICATION_CLIENT);
		} catch (GLib.Error e) {
			GLib.warning("Unable to connect to dbus server at '%s': %s", address, e.message);
			/* If it fails, it means the dbus pulse extension is not available */
		}
		GLib.debug ("PulseAudio dbus address is '%s', connection is '%p'", address, conn);
		return conn;
	}

	/* PulseAudio Dbus (Stream Restore) logic */
	private void reconnect_pulse_dbus ()
	{
		/* In case of a reconnect */
		_pulse_use_stream_restore = false;
		_pa_volume_sig_count = 0;

		_pconn = create_pulse_dbus_connection();
		if (_pconn == null)
			return;

		/* For pulse dbus related events */
		_pconn.add_filter (pulse_dbus_filter);

		/* Check if the 4 currently supported media roles are already available in StreamRestore
		 * Roles: multimedia, alert, alarm and phone */
		_objp_role_multimedia = stream_restore_get_object_path (_pconn, "sink-input-by-media-role:multimedia");
		_objp_role_alert = stream_restore_get_object_path (_pconn, "sink-input-by-media-role:alert");
		_objp_role_alarm = stream_restore_get_object_path (_pconn, "sink-input-by-media-role:alarm");
		_objp_role_phone = stream_restore_get_object_path (_pconn, "sink-input-by-media-role:phone");

		/* Only use stream restore if every used role is available */
		if (_objp_role_multimedia != null && _objp_role_alert != null && _objp_role_alarm != null && _objp_role_phone != null) {
			debug ("Using PulseAudio DBUS Stream Restore module");
			/* Restore volume and update default entry */
			update_active_sink_input.begin (-1);
			_pulse_use_stream_restore = true;
		}
	}

	public static string? stream_restore_get_object_path (DBusConnection pconn, string name) {
		string? objp = null;
		try {
			Variant props_variant = pconn.call_sync ("org.PulseAudio.Ext.StreamRestore1",
					"/org/pulseaudio/stream_restore1", "org.PulseAudio.Ext.StreamRestore1",
					"GetEntryByName", new Variant ("(s)", name), null, DBusCallFlags.NONE, -1);
			/* Workaround for older versions of vala that don't provide get_objv */
			VariantIter iter = props_variant.iterator ();
			iter.next ("o", &objp);
			debug ("Found obj path %s for restore data named %s\n", objp, name);
		} catch (GLib.Error e) {
			warning ("unable to find stream restore data for: %s", name);
		}
		return objp;
	}

	/* AccountsService operations */
	private void accountsservice_props_changed_cb (DBusProxy proxy, Variant changed_properties, string[]? invalidated_properties)
	{
		Variant volume_variant = changed_properties.lookup_value ("Volume", VariantType.DOUBLE);
		if (volume_variant != null) {
			var volume = volume_variant.get_double ();
			if (volume >= 0) {
				_account_service_volume = volume;
				// we need to wait for this to settle.
				start_account_service_volume_timer();
			}
		}

		Variant mute_variant = changed_properties.lookup_value ("Muted", VariantType.BOOLEAN);
		if (mute_variant != null) {
			var mute = mute_variant.get_boolean ();
			set_mute_internal (mute);
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
				warning ("unable to find Accounts path for user %s: %s", username, e.message);
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
			user_path_variant.get ("(o)", out user_path);
			_user_proxy = yield DBusProxy.create_for_bus (BusType.SYSTEM, DBusProxyFlags.GET_INVALIDATED_PROPERTIES, null, "org.freedesktop.Accounts", user_path, "com.ubuntu.AccountsService.Sound");
		} catch (GLib.Error e) {
			warning ("unable to find Accounts path for user %s: %s", username, e.message);
			return;
		}

		// Get current values and listen for changes
		_user_proxy.g_properties_changed.connect (accountsservice_props_changed_cb);
		try {
			var props_variant = yield _user_proxy.get_connection ().call (_user_proxy.get_name (), _user_proxy.get_object_path (), "org.freedesktop.DBus.Properties", "GetAll", new Variant ("(s)", _user_proxy.get_interface_name ()), null, DBusCallFlags.NONE, -1);
			Variant props;
			props_variant.get ("(@a{sv})", out props);
			accountsservice_props_changed_cb(_user_proxy, props, null);
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
			if (username != "" && username != null) {
				yield setup_user_proxy (username);
			}
		}
	}

	private async void sync_mute_to_accountsservice (bool mute)
	{
		if (_user_proxy == null)
			return;

		_mute_cancellable.cancel ();
		_mute_cancellable.reset ();

		try {
			yield _user_proxy.get_connection ().call (_user_proxy.get_name (), _user_proxy.get_object_path (), "org.freedesktop.DBus.Properties", "Set", new Variant ("(ssv)", _user_proxy.get_interface_name (), "Muted", new Variant ("b", mute)), null, DBusCallFlags.NONE, -1, _mute_cancellable);
		} catch (GLib.Error e) {
			warning ("unable to sync mute to AccountsService: %s", e.message);
		}
	}

	private async void sync_volume_to_accountsservice (VolumeControl.Volume volume)
	{
		if (_user_proxy == null)
			return;

		_volume_cancellable.cancel ();
		_volume_cancellable.reset ();

		try {
			yield _user_proxy.get_connection ().call (_user_proxy.get_name (), _user_proxy.get_object_path (), "org.freedesktop.DBus.Properties", "Set", new Variant ("(ssv)", _user_proxy.get_interface_name (), "Volume", new Variant ("d", volume.volume)), null, DBusCallFlags.NONE, -1, _volume_cancellable);
		} catch (GLib.Error e) {
			warning ("unable to sync volume to AccountsService: %s", e.message);
		}
	}

	private void start_local_volume_timer()
	{
		// perform a slow sync with the accounts service. max at 1 per second.

		// stop the AS update timer, as since we're going to be setting the volume.
		stop_account_service_volume_timer();

		if (_local_volume_timer == 0) {
			sync_volume_to_accountsservice.begin (_volume);
			_local_volume_timer = Timeout.add_seconds (1, local_volume_changed_timeout);
		} else {
			_send_next_local_volume = true;
		}
	}

	private void stop_local_volume_timer()
	{
		if (_local_volume_timer != 0) {
			Source.remove (_local_volume_timer);
			_local_volume_timer = 0;
		}
	}

	bool local_volume_changed_timeout()
	{
		_local_volume_timer = 0;
		if (_send_next_local_volume) {
			_send_next_local_volume = false;
			start_local_volume_timer ();
		}
		return Source.REMOVE;
	}

	private void start_account_service_volume_timer()
	{
		if (_accountservice_volume_timer == 0) {
			// If we haven't been messing with local volume recently, apply immediately.
			if (_local_volume_timer == 0) {
				var vol = new VolumeControl.Volume();
				vol.volume = _account_service_volume;
				vol.reason = VolumeControl.VolumeReasons.ACCOUNTS_SERVICE_SET;
				this.volume = vol;
				return;
			}
			// Else check again in another second if needed.
			// (if AS is throwing us lots of notifications, we update at most once a second)
			_accountservice_volume_timer = Timeout.add_seconds (1, accountservice_volume_changed_timeout);
		}
	}

	private void stop_account_service_volume_timer()
	{
		if (_accountservice_volume_timer != 0) {
			Source.remove (_accountservice_volume_timer);
			_accountservice_volume_timer = 0;
		}
	}

	bool accountservice_volume_changed_timeout ()
	{
		_accountservice_volume_timer = 0;
		start_account_service_volume_timer ();
		return Source.REMOVE;
	}
}
