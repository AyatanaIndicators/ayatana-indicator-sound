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

[CCode(cname="pa_cvolume_set", cheader_filename = "pulse/volume.h")]
extern unowned PulseAudio.CVolume? vol_set2 (PulseAudio.CVolume? cv, uint channels, PulseAudio.Volume v);

public class VolumeWarning : Object
{
	// true if the warning dialog is currently active
	public bool active { get; public set; default = false; }

	// true iff we're playing unapproved loud multimedia over headphones
	public bool high_volume { get; protected set; default = false; }

	/* this is static to ensure it being freed after @context (loop does not have ref counting) */
	private static PulseAudio.GLibMainLoop loop;

	private uint _reconnect_timer = 0;

	private PulseAudio.Context context;
	private bool   _ignore_warning_this_time = false;
	private VolumeControl.Volume _volume = new VolumeControl.Volume();
	private Settings _settings = new Settings ("com.canonical.indicator.sound");

	/* Used by the pulseaudio stream restore extension */
	private DBusConnection _pconn;
	/* Need both the list and hash so we can retrieve the last known sink-input after
	 * releasing the current active one (restoring back to the previous known role) */
	private Gee.ArrayList<uint32> _sink_input_list = new Gee.ArrayList<uint32> ();
	private HashMap<uint32, string> _sink_input_hash = new HashMap<uint32, string> ();
	private bool _pulse_use_stream_restore = false;
	private int32 _active_sink_input = -1;
	private string[] _valid_roles = {"multimedia", "alert", "alarm", "phone"};
	private string stream {
		get {
			if (_active_sink_input == -1)
				return "alert";
			var path = _sink_input_hash[_active_sink_input];
			if (path == _objp_role_multimedia)
				return "multimedia";
			if (path == _objp_role_alert)
				return "alert";
			if (path == _objp_role_alarm)
				return "alarm";
			if (path == _objp_role_phone)
				return "phone";
			return "alert";
		}
	}
	private string? _objp_role_multimedia = null;
	private string? _objp_role_alert = null;
	private string? _objp_role_alarm = null;
	private string? _objp_role_phone = null;
	private uint _pa_volume_sig_count = 0;

	private bool _active_port_headphones = false;
	private VolumeControl.ActiveOutput _active_output = VolumeControl.ActiveOutput.SPEAKERS;
	private IndicatorSound.Options _options;

	public VolumeWarning (IndicatorSound.Options options)
	{
		_options = options;

		_volume.volume = 0.0;
		_volume.reason = VolumeControl.VolumeReasons.PULSE_CHANGE;

		if (loop == null)
			loop = new PulseAudio.GLibMainLoop ();

		init_all_properties();

		this.reconnect_to_pulse ();

		_notification = new IndicatorSound.WarnNotification();
		_notification.user_responded.connect((n, response) => on_user_response(response));
        }

	private void init_all_properties()
	{
		init_high_volume();
		init_high_volume_approved();
	}

	~VolumeWarning ()
	{
		stop_all_timers();
	}

	private void stop_all_timers()
	{
		if (_reconnect_timer != 0) {
			Source.remove (_reconnect_timer);
			_reconnect_timer = 0;
		}
		stop_high_volume_approved_timer();
		stop_clamp_to_loud_timeout();
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
			case Context.SubscriptionEventType.SOURCE_OUTPUT:
				break;
		}
	}

	private void sink_info_cb_for_props (Context c, SinkInfo? i, int eol)
	{
		if (i == null)
			return;

		var old_active_output = active_output;
		var new_active_output = VolumeControlPulse.calculate_active_output(i);

		_active_output = new_active_output;

		switch (new_active_output) {
			case VolumeControl.ActiveOutput.HEADPHONES:
			case VolumeControl.ActiveOutput.USB_HEADPHONES:
			case VolumeControl.ActiveOutput.HDMI_HEADPHONES:
			case VolumeControl.ActiveOutput.BLUETOOTH_HEADPHONES:
				_active_port_headphones = true;
				break;

			default:
				_active_port_headphones = false;
				break;
		}

		if ((new_active_output != old_active_output)
				&& (new_active_output != VolumeControl.ActiveOutput.CALL_MODE)
				&& (old_active_output != VolumeControl.ActiveOutput.CALL_MODE))
			update_high_volume();

		if (_pulse_use_stream_restore == false &&
				_volume.volume != volume_to_double (i.volume.max ()))
		{
			var vol = new VolumeControl.Volume();
			vol.volume = volume_to_double (i.volume.max ());
			vol.reason = VolumeControl.VolumeReasons.PULSE_CHANGE;
			this.volume = vol;
		}
	}

	private void server_info_cb_for_props (Context c, ServerInfo? i)
	{
		if (i != null)
			context.get_sink_info_by_name (i.default_sink_name, sink_info_cb_for_props);
	}

	private void update_sink ()
	{
		context.get_server_info (server_info_cb_for_props);
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
						// Ignore changes from PULSE to avoid issues with
						// some apps that change the volume in the sink
						// We only take into account volume changes from the user
						this._ignore_warning_this_time = true;
						this.volume = vol;
					}
				}
			}
		}

		return message;
	}

	private async void update_active_sink_input (int32 index)
	{
		if ((index == -1) || (index != _active_sink_input && index in _sink_input_list)) {
			string sink_input_objp = _objp_role_alert;
			if (index != -1)
				sink_input_objp = _sink_input_hash.get (index);
			_active_sink_input = index;

			/* Listen for role volume changes from pulse itself (external clients) */
			try {
				var builder = new VariantBuilder (new VariantType ("ao"));
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
				// Ignore changes from PULSE to avoid issues with
                                // some apps that change the volume in the sink
                                // We only take into account volume changes from the user
				this._ignore_warning_this_time = true;
				this.volume = vol;
			} catch (GLib.Error e) {
				warning ("unable to get volume for active role %s (%s)", sink_input_objp, e.message);
			}
		}
	}

	private void add_sink_input_into_list (SinkInputInfo sink_input)
	{
		/* We're only adding ones that are not corked and with a valid role */
		var role = sink_input.proplist.gets (PulseAudio.Proplist.PROP_MEDIA_ROLE);

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

	private bool _connected_to_pulse = false;

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
				_connected_to_pulse = true;
				break;

			case Context.State.FAILED:
			case Context.State.TERMINATED:
				if (_reconnect_timer == 0)
					_reconnect_timer = Timeout.add_seconds (2, reconnect_timeout);
				break;

			default:
				_connected_to_pulse = false;
				break;
		}
	}

	bool reconnect_timeout ()
	{
		_reconnect_timer = 0;
		reconnect_to_pulse ();
		return false; // G_SOURCE_REMOVE
	}

	void reconnect_to_pulse ()
	{
		if (_connected_to_pulse) {
			this.context.disconnect ();
			this.context = null;
			_connected_to_pulse = false;
		}

		var props = new Proplist ();
		props.sets (Proplist.PROP_APPLICATION_NAME, "Ubuntu Audio Settings");
		props.sets (Proplist.PROP_APPLICATION_ID, "com.canonical.settings.sound");
		props.sets (Proplist.PROP_APPLICATION_ICON_NAME, "multimedia-volume-control");
		props.sets (Proplist.PROP_APPLICATION_VERSION, "0.1");

		reconnect_pulse_dbus ();

		this.context = new PulseAudio.Context (loop.get_api(), null, props);
		this.context.set_state_callback (context_state_callback);

		var server_string = Environment.get_variable("PULSE_SERVER");
		if (context.connect(server_string, Context.Flags.NOFAIL, null) < 0)
			warning( "pa_context_connect() failed: %s\n", PulseAudio.strerror(context.errno()));
	}

	private VolumeControl.ActiveOutput active_output
	{
		get
		{
			return _active_output;
		}
	}

	/* Volume operations */
	private static PulseAudio.Volume double_to_volume (double vol)
	{
		double tmp = (double)(PulseAudio.Volume.NORM - PulseAudio.Volume.MUTED) * vol;
		return (PulseAudio.Volume)tmp + PulseAudio.Volume.MUTED;
	}

	private static double volume_to_double (PulseAudio.Volume vol)
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

			debug ("Set volume to %f on path %s", vol, active_role_objp);
		} catch (GLib.Error e) {
			lock (_pa_volume_sig_count) {
				_pa_volume_sig_count--;
			}
			warning ("unable to set volume for stream obj path %s (%s)", active_role_objp, e.message);
		}
	}

	private VolumeControl.Volume volume {
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

			update_high_volume();
		}
	}

	/** HIGH VOLUME PROPERTY **/

	public bool ignore_high_volume {
		get {
			if (_ignore_warning_this_time) {
				warning("Ignore");
				_ignore_warning_this_time = false;
				return true;
			}
			return false;
		}
		set { }
	}
	private void init_high_volume() {
		_options.loud_changed.connect(() => update_high_volume());
		update_high_volume();
	}
	private void update_high_volume() {
		var new_high_volume = calculate_high_volume();
		if (high_volume != new_high_volume) {
			debug("changing high_volume from %d to %d", (int)high_volume, (int)new_high_volume);
			high_volume = new_high_volume;
			if (high_volume && !active)
				show(_volume);
		}
	}
	private bool calculate_high_volume() {
		return calculate_high_volume_from_volume(_volume.volume);
	}
	private bool calculate_high_volume_from_volume(double volume) {
		return _active_port_headphones
			&& _options.is_loud(_volume)
			&& (stream == "multimedia");
	}

	public void set_warning_volume() {
		var vol = new VolumeControl.Volume();
                vol.volume = volume_to_double(_options.loud_volume());
                vol.reason = _volume.reason;
                debug("Setting warning level volume from %f down to %f", _volume.volume, vol.volume);
                volume = vol;
	}

	/** HIGH VOLUME APPROVED PROPERTY **/

	public bool high_volume_approved { get; private set; default = false; }

	private void approve_high_volume() {
		_high_volume_approved_at = GLib.get_monotonic_time();
		update_high_volume_approved();
		update_high_volume_approved_timer();
	}

	private uint _high_volume_approved_timer = 0;
	private int64 _high_volume_approved_at = 0;
	private int64 _high_volume_approved_ttl_usec = 0;
	private void init_high_volume_approved() {
		_settings.changed["warning-volume-confirmation-ttl"].connect(() => update_high_volume_approved_cache());
		update_high_volume_approved_cache();
	}
	private void update_high_volume_approved_cache() {
		_high_volume_approved_ttl_usec = _settings.get_int("warning-volume-confirmation-ttl");
		_high_volume_approved_ttl_usec *= 1000000;

		update_high_volume_approved();
		update_high_volume_approved_timer();
	}
	private void update_high_volume_approved_timer() {
		stop_high_volume_approved_timer();
		if (_high_volume_approved_at != 0) {
			int64 expires_at = _high_volume_approved_at + _high_volume_approved_ttl_usec;
			int64 now = GLib.get_monotonic_time();
			if (expires_at > now) {
				var seconds_left = 1 + ((expires_at - now) / 1000000);
				_high_volume_approved_timer = Timeout.add_seconds((uint)seconds_left, on_high_volume_approved_timer);
			}
		}
	}
	private void stop_high_volume_approved_timer() {
		if (_high_volume_approved_timer != 0) {
			Source.remove (_high_volume_approved_timer);
			_high_volume_approved_timer = 0;
		}
	}
	private bool on_high_volume_approved_timer() {
		_high_volume_approved_timer = 0;
		update_high_volume_approved();
		return false; /* Source.REMOVE */
	}
	private void update_high_volume_approved() {
		var new_high_volume_approved = calculate_high_volume_approved();
		if (high_volume_approved != new_high_volume_approved) {
			debug("changing high_volume_approved from %d to %d", (int)high_volume_approved, (int)new_high_volume_approved);
			high_volume_approved = new_high_volume_approved;
		}
	}
	private bool calculate_high_volume_approved() {
		int64 now = GLib.get_monotonic_time();
		return (_high_volume_approved_at != 0)
			&& (_high_volume_approved_at + _high_volume_approved_ttl_usec >= now);
	}

	/* PulseAudio Dbus (Stream Restore) logic */
	private void reconnect_pulse_dbus ()
	{
		/* In case of a reconnect */
		_pulse_use_stream_restore = false;
		_pa_volume_sig_count = 0;

		_pconn = VolumeControlPulse.create_pulse_dbus_connection();
		if (_pconn == null)
			return;

		/* For pulse dbus related events */
		_pconn.add_filter (pulse_dbus_filter);

		/* Check if the 4 currently supported media roles are already available in StreamRestore
		 * Roles: multimedia, alert, alarm and phone */
		_objp_role_multimedia = stream_restore_get_object_path ("sink-input-by-media-role:multimedia");
		_objp_role_alert = stream_restore_get_object_path ("sink-input-by-media-role:alert");
		_objp_role_alarm = stream_restore_get_object_path ("sink-input-by-media-role:alarm");
		_objp_role_phone = stream_restore_get_object_path ("sink-input-by-media-role:phone");

		/* Only use stream restore if every used role is available */
		if (_objp_role_multimedia != null && _objp_role_alert != null && _objp_role_alarm != null && _objp_role_phone != null) {
			debug ("Using PulseAudio DBUS Stream Restore module");
			/* Restore volume and update default entry */
			update_active_sink_input.begin (-1);
			_pulse_use_stream_restore = true;
		}
	}

	private string? stream_restore_get_object_path (string name) {
		string? objp = null;
		try {
			Variant props_variant = _pconn.call_sync ("org.PulseAudio.Ext.StreamRestore1",
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

	private void set_multimedia_volume(VolumeControl.Volume volume)
	{
		// FIXME
	}

	// NOTIFICATION

	private IndicatorSound.WarnNotification _notification = new IndicatorSound.WarnNotification();

	private VolumeControl.Volume _cancel_volume = null;
	private VolumeControl.Volume _ok_volume = null;

	private void show(VolumeControl.Volume volume) {

		// the volume to use if user hits 'cancel'
		_cancel_volume = new VolumeControl.Volume();
		_cancel_volume.volume = VolumeControlPulse.volume_to_double(_options.loud_volume());
		_cancel_volume.reason = VolumeControl.VolumeReasons.USER_KEYPRESS;

		// the volume to use if user hits 'ok'
		_ok_volume = new VolumeControl.Volume();
		_ok_volume.volume = volume.volume;
		_ok_volume.reason = VolumeControl.VolumeReasons.USER_KEYPRESS;

		_notification.show();
		this.active = true;
	}

	private void on_user_response(IndicatorSound.WarnNotification.Response response) {
		_notification.close();
		stop_clamp_to_loud_timeout();

		if (response == IndicatorSound.WarnNotification.Response.OK) {
			approve_high_volume();
			set_multimedia_volume(_ok_volume);
		} else { // WarnNotification.CANCEL
			set_multimedia_volume(_cancel_volume);
		}

		_cancel_volume = null;
		_ok_volume = null;

		this.active = false;
	}

	public enum Key {
		VOLUME_UP,
		VOLUME_DOWN
	}

	public void user_keypress(Key key) {
		if (key == Key.VOLUME_DOWN)
			on_user_response(IndicatorSound.WarnNotification.Response.CANCEL);
	}

	// VOLUME CLAMPING

	private uint _clamp_to_loud_timeout = 0;

	private void stop_clamp_to_loud_timeout() {
		if (_clamp_to_loud_timeout != 0) {
			Source.remove(_clamp_to_loud_timeout);
			_clamp_to_loud_timeout = 0;
		}
	}

	private void clamp_to_loud_soon() {
		const uint interval_msec = 200;
		if (_clamp_to_loud_timeout == 0)
			_clamp_to_loud_timeout = Timeout.add(interval_msec, clamp_to_loud_idle);
	}

	private bool clamp_to_loud_idle() {
		_clamp_to_loud_timeout = 0;
		clamp_to_loud_volume();
		return false; // Source.REMOVE;
	}

	private void clamp_to_loud_volume() {
		if ((_cancel_volume != null) && (_volume.volume > _cancel_volume.volume)) {
			debug("Clamping from %f down to %f", _volume.volume, _cancel_volume.volume);
			set_multimedia_volume (_cancel_volume);
		}
	}
}
