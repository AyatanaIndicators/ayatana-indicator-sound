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

	// true if we're playing unapproved loud multimedia over headphones
	public bool high_volume { get; protected set; default = false; }

	// true if the active sink input has its role property set to multimedia
	protected bool multimedia_active { get; set; default = false; }

	// true if the user has approved high volumes recently
	protected bool high_volume_approved { get; set; default = false; }

	protected PulseAudio.Volume multimedia_volume { get; set; default = PulseAudio.Volume.MUTED; }

	public enum Key {
		VOLUME_UP,
		VOLUME_DOWN
	}

	public void user_keypress(Key key) {
		if (key == Key.VOLUME_DOWN)
			on_user_response(IndicatorSound.WarnNotification.Response.CANCEL);
	}

	public VolumeWarning (IndicatorSound.Options options)
	{
		_options = options;

		if (loop == null)
			loop = new PulseAudio.GLibMainLoop ();

		init_all_properties();

		this.reconnect_to_pulse ();

		_notification = new IndicatorSound.WarnNotification();
		_notification.user_responded.connect((n, response) => on_user_response(response));
        }

	/***
	****
	***/

	/* this is static to ensure it being freed after @context (loop does not have ref counting) */
	private static PulseAudio.GLibMainLoop loop;

	private PulseAudio.Context context;
	// FIXME: what to do with this now?
	private bool   _ignore_warning_this_time = false;
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

	private string? _objp_role_multimedia = null;
	private string? _objp_role_alert = null;
	private string? _objp_role_alarm = null;
	private string? _objp_role_phone = null;
	private uint _pa_volume_sig_count = 0;

	private bool _active_port_headphones = false;
	private VolumeControl.ActiveOutput _active_output = VolumeControl.ActiveOutput.SPEAKERS;
	private IndicatorSound.Options _options;

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
		stop_reconnect_timer();
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
		if (message.get_message_type() == DBusMessageType.SIGNAL)
		{
			var member = message.get_member();
			var path = message.get_path();

			if ((member == "VolumeUpdated") && (path == _multimedia_objp))
			{
				Variant body = message.get_body ();
				Variant varray = body.get_child_value (0);
				uint32 type = 0, lvolume = 0;
				VariantIter iter = varray.iterator ();
				iter.next ("(uu)", &type, &lvolume);
				if (multimedia_volume != lvolume)
					multimedia_volume = lvolume;
			}
			else if (((member == "NewEntry") || (member == "EntryRemoved")) && (path == "/org/pulseaudio/stream_restore1"))
			{
				update_multimedia_objp();
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

			multimedia_active = (index != -1)
				&& (_sink_input_hash.get(index) == _objp_role_multimedia);

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

	private uint _reconnect_timer = 0;

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

	private void stop_reconnect_timer()
	{
		if (_reconnect_timer != 0) {
			Source.remove (_reconnect_timer);
			_reconnect_timer = 0;
		}
	}

	bool reconnect_timeout ()
	{
		_reconnect_timer = 0;
		reconnect_to_pulse ();
		return false; // G_SOURCE_REMOVE
	}

	private void reconnect_to_pulse ()
	{
		if (_connected_to_pulse) {
			this.context.disconnect ();
			this.context = null;
			_connected_to_pulse = false;
		}

		reconnect_pulse_dbus ();

		var props = new Proplist ();
		props.sets (Proplist.PROP_APPLICATION_NAME, "Ubuntu Audio Settings");
		props.sets (Proplist.PROP_APPLICATION_ID, "com.canonical.settings.sound");
		props.sets (Proplist.PROP_APPLICATION_ICON_NAME, "multimedia-volume-control");
		props.sets (Proplist.PROP_APPLICATION_VERSION, "0.1");
		this.context = new PulseAudio.Context (loop.get_api(), null, props);
		this.context.set_state_callback (context_state_callback);

		var server_string = Environment.get_variable("PULSE_SERVER");
		if (context.connect(server_string, Context.Flags.NOFAIL, null) < 0)
			warning( "pa_context_connect() failed: %s\n", PulseAudio.strerror(context.errno()));
	}

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

		// track the multimedia object path
		init_stream_restore.begin();

		/* Check if the 4 currently supported media roles are already available in StreamRestore
		 * Roles: multimedia, alert, alarm and phone */
		_objp_role_multimedia = VolumeControlPulse.stream_restore_get_object_path (_pconn, "sink-input-by-media-role:multimedia");
		_objp_role_alert = VolumeControlPulse.stream_restore_get_object_path (_pconn, "sink-input-by-media-role:alert");
		_objp_role_alarm = VolumeControlPulse.stream_restore_get_object_path (_pconn, "sink-input-by-media-role:alarm");
		_objp_role_phone = VolumeControlPulse.stream_restore_get_object_path (_pconn, "sink-input-by-media-role:phone");

		/* Only use stream restore if every used role is available */
		if (_objp_role_multimedia != null && _objp_role_alert != null && _objp_role_alarm != null && _objp_role_phone != null) {
			debug ("Using PulseAudio DBUS Stream Restore module");
			/* Restore volume and update default entry */
			update_active_sink_input.begin (-1);
			_pulse_use_stream_restore = true;
		}
	}

	/***
	****  Tracking the Multimedia Volume
	***/

	private async void init_stream_restore()
	{
		if (_pconn == null)
			return;

		update_multimedia_objp();

		// listen for StreamRestore1's NewEntry and EntryRemoved signals
		try {
			var builder = new VariantBuilder (new VariantType ("ao"));
			builder.add ("o", "/org/pulseaudio/stream_restore1");
			yield _pconn.call ("org.PulseAudio.Core1", "/org/pulseaudio/core1",
					"org.PulseAudio.Core1", "ListenForSignal",
					new Variant ("(sao)", "org.PulseAudio.Ext.StreamRestore1.NewEntry", builder),
					null, DBusCallFlags.NONE, -1);

			builder = new VariantBuilder (new VariantType ("ao"));
			builder.add ("o", "/org/pulseaudio/stream_restore1");
			yield _pconn.call ("org.PulseAudio.Core1", "/org/pulseaudio/core1",
					"org.PulseAudio.Core1", "ListenForSignal",
					new Variant ("(sao)", "org.PulseAudio.Ext.StreamRestore1.EntryRemoved", builder),
					null, DBusCallFlags.NONE, -1);
		} catch (GLib.Error e) {
			warning ("unable to listen for StreamRestore1 dbus signals (%s)", e.message);
		}
	}

	private void update_multimedia_objp()
	{
		var objp = VolumeControlPulse.stream_restore_get_object_path(
			_pconn,
			"sink-input-by-media-role:multimedia");
		set_multimedia_object_path.begin(objp);
	}

	private string _multimedia_objp = null;

	private async void set_multimedia_object_path(string objp)
	{
		if (_multimedia_objp == objp)
			return;

		_multimedia_objp = objp;

		// listen for RestoreEntry.VolumeUpdated from this entry
		try {
			var builder = new VariantBuilder (new VariantType ("ao"));
			builder.add ("o", _multimedia_objp);
			yield _pconn.call ("org.PulseAudio.Core1", "/org/pulseaudio/core1",
				"org.PulseAudio.Core1", "ListenForSignal",
				new Variant ("(sao)", "org.PulseAudio.Ext.StreamRestore1.RestoreEntry.VolumeUpdated", builder),
				null, DBusCallFlags.NONE, -1);
		} catch (GLib.Error e) {
			warning ("unable to listen for RestoreEntry dbus signals (%s)", e.message);
		}

		update_multimedia_volume.begin();
	}

	private async void update_multimedia_volume()
	{
		try {
			var props_variant = yield _pconn.call (
				"org.PulseAudio.Ext.StreamRestore1.RestoreEntry",
				_multimedia_objp,
				"org.freedesktop.DBus.Properties",
				"Get",
				new Variant ("(ss)", "org.PulseAudio.Ext.StreamRestore1.RestoreEntry", "Volume"),
				null,
				DBusCallFlags.NONE,
				-1);
                                Variant tmp;
			props_variant.get ("(v)", out tmp);
			uint32 type = 0, volume = 0;
			VariantIter iter = tmp.iterator ();
			iter.next ("(uu)", &type, &volume);
			if (multimedia_volume != volume)
				multimedia_volume = volume;
		} catch (GLib.Error e) {
			warning ("unable to get volume for multimedia role %s (%s)", _multimedia_objp, e.message);
		}
	}

	/***
	****  Tracking the Active Output
	***/

	private VolumeControl.ActiveOutput active_output
	{
		get
		{
			return _active_output;
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
				show();
		}
	}
	private bool calculate_high_volume() {
		return calculate_high_volume_from_volume(multimedia_volume);
	}
	private bool calculate_high_volume_from_volume(PulseAudio.Volume volume) {
		return _active_port_headphones
			&& _options.is_loud_pulse(volume)
			&& multimedia_active;
	}

	/** HIGH VOLUME APPROVED PROPERTY **/

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

	protected virtual async void set_pulse_multimedia_volume(PulseAudio.Volume volume)
	{
		var objp = _multimedia_objp;
		if (objp == null)
			return;

		try {
			var builder = new VariantBuilder (new VariantType ("a(uu)"));
			builder.add ("(uu)", 0, volume);

			yield _pconn.call ("org.PulseAudio.Ext.StreamRestore1.RestoreEntry",
					objp, "org.freedesktop.DBus.Properties", "Set",
					new Variant ("(ssv)", "org.PulseAudio.Ext.StreamRestore1.RestoreEntry", "Volume", builder),
					null, DBusCallFlags.NONE, -1);

			debug ("Set multimedia volume to %d on path %s", (int)volume, objp);
		} catch (GLib.Error e) {
			warning ("unable to set volume for stream obj path %s (%s)", objp, e.message);
		}
	}

	// NOTIFICATION

	private IndicatorSound.WarnNotification _notification = new IndicatorSound.WarnNotification();

	private PulseAudio.Volume _cancel_volume = PulseAudio.Volume.INVALID;
	private PulseAudio.Volume _ok_volume = PulseAudio.Volume.INVALID;

	private void show() {
		_cancel_volume = _options.loud_volume();
		_ok_volume = multimedia_volume;
		_notification.show();
		this.active = true;
	}

	private void on_user_response(IndicatorSound.WarnNotification.Response response) {
		_notification.close();
		stop_clamp_to_loud_timeout();

		if (response == IndicatorSound.WarnNotification.Response.OK) {
			approve_high_volume();
			set_pulse_multimedia_volume.begin(_ok_volume);
		} else { // WarnNotification.CANCEL
			set_pulse_multimedia_volume.begin(_cancel_volume);
		}

		_cancel_volume = PulseAudio.Volume.INVALID;
		_ok_volume = PulseAudio.Volume.INVALID;

		this.active = false;
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
		var loud_volume = _options.loud_volume();
		if (multimedia_volume > loud_volume) {
			debug("Clamping from %d down to %d", (int)multimedia_volume, (int)loud_volume);
			set_pulse_multimedia_volume.begin (loud_volume);
		}
	}
}
