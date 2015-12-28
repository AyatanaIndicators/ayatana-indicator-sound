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

public class VolumeWarning : Object
{
	// true if the active sink input has its role property set to multimedia
	public bool multimedia_active { get; set; default = false; }

	// true if headphones are currently in use
	public bool headphones_active { get; set; default = false; }

	// true if the warning dialog is currently active
	public bool active { get; protected set; default = false; }

	// true if we're playing unapproved loud multimedia over headphones
	public bool high_volume { get; protected set; default = false; }

	public enum Key {
		VOLUME_UP,
		VOLUME_DOWN
	}

	public void user_keypress(Key key) {
		if (key == Key.VOLUME_DOWN)
			on_user_response(IndicatorSound.WarnNotification.Response.CANCEL);
	}

	public VolumeWarning (IndicatorSound.Options options, PulseAudio.GLibMainLoop pgloop) {
		_options = options;
		_pgloop = pgloop;

		init_all_properties();

		pulse_start();

		_notification = new IndicatorSound.WarnNotification();
		_notification.user_responded.connect((n, response) => on_user_response(response));
	}

	/***
	****
	***/

	// true if the user has approved high volumes recently
	protected bool high_volume_approved { get; set; default = false; }

	/* The multimedia volume.
	   NB: this is a PulseAudio.Volume type in all but name.
	   The next line says 'uint' to unconfuse valac's code generator */
	protected uint multimedia_volume { get; set; default = PulseAudio.Volume.MUTED; }

	protected virtual async void set_pulse_multimedia_volume(PulseAudio.Volume volume)
	{
#if 0
		var objp = _multimedia_objp;
		if (objp == null)
			return;

		try {
                        var builder = new VariantBuilder (new VariantType ("a(uu)"));
                        builder.add ("(uu)", 0, volume);
                        var volvar = builder.end ();

                        GLib.message ("Setting multimedia volume to %s on path %s", volvar.print(true), objp);
                        yield _pconn.call ("org.PulseAudio.Ext.StreamRestore1.RestoreEntry",
                                        objp, "org.freedesktop.DBus.Properties", "Set",
					new Variant ("(ssv)", "org.PulseAudio.Ext.StreamRestore1.RestoreEntry", "Volume", volvar),
					null, DBusCallFlags.NONE, -1);

			debug ("Set multimedia volume to %d on path %s", (int)volume, objp);
		} catch (GLib.Error e) {
			warning ("unable to set multimedia volume for stream obj path %s (%s)", objp, e.message);
		}
#endif
	}

	/***
	****
	***/

	// FIXME: what to do with this now?
	private bool   _ignore_warning_this_time = false;

#if 0
	/* Used by the pulseaudio stream restore extension */
	private DBusConnection _pconn;
#endif

	private IndicatorSound.Options _options;

	private void init_all_properties()
	{
		init_high_volume();
		init_high_volume_approved();
	}

	~VolumeWarning ()
	{
		stop_all_timers();

		pulse_stop();
	}

	private void stop_all_timers()
	{
		stop_high_volume_approved_timer();
        	stop_show_timeout();
	}

	/***
	****  PulseAudio: Tracking which sink input (if any) is active multimedia
	***/

	private unowned PulseAudio.GLibMainLoop _pgloop = null;
	private PulseAudio.Context _pulse_context = null;
	private uint _pulse_reconnect_timer = 0;
	private uint32 _multimedia_sink_input_index = PulseAudio.INVALID_INDEX;

	private bool is_active_multimedia (SinkInputInfo i)
	{
		if (i.corked != 0)
			return false;

		GLib.message("proplist: %s", i.proplist.to_string());
		var media_role = i.proplist.gets(PulseAudio.Proplist.PROP_MEDIA_ROLE);
		if (media_role != "multimedia")
			return false;

		return true;
	}

	private PulseAudio.Operation _sink_input_info_list_operation = null;

	private void pulse_on_sink_input_info (Context c, SinkInputInfo? i, int eol)
	{
		if (i == null)
			return;

		bool active = is_active_multimedia(i);

		if (active)
			_multimedia_sink_input_index = i.index;
		else if (i.index == _multimedia_sink_input_index)
			_multimedia_sink_input_index = PulseAudio.INVALID_INDEX;

		if (active) {
			GLib.message("index %d", (int)i.index);
			GLib.message("name %s", i.name);
			GLib.message("sink %d", (int)i.sink);
			GLib.message("has_volume %d", (int)i.has_volume);
			GLib.message("volume %s", i.volume.to_string());
			GLib.message("driver %s", i.driver);
		}

		if (eol != 0)
			GLib.message("at end of list, _multimedia_sink_input_index is %d", (int)_multimedia_sink_input_index);
	}

	private void pulse_update_sink_inputs_cancel()
	{
		if (_sink_input_info_list_operation != null)
		{
			_sink_input_info_list_operation.cancel();
			_sink_input_info_list_operation = null;
		}
	}

	private void pulse_update_sink_inputs()
	{
		GLib.message("list_input_sinks");

		pulse_update_sink_inputs_cancel ();

		_sink_input_info_list_operation = _pulse_context.get_sink_input_info_list (pulse_on_sink_input_info);
	}


	private void context_events_cb (Context c, Context.SubscriptionEventType t, uint32 index)
	{
		GLib.message("");
		if ((t & Context.SubscriptionEventType.FACILITY_MASK) != Context.SubscriptionEventType.SINK_INPUT)
			return;

		switch (t & Context.SubscriptionEventType.TYPE_MASK)
		{
			case Context.SubscriptionEventType.NEW:
				GLib.message("Context.SubscriptionEventType.NEW");
				pulse_update_sink_inputs();
				break;
			case Context.SubscriptionEventType.CHANGE:
				GLib.message("Context.SubscriptionEventType.CHANGE");
				pulse_update_sink_inputs();
				break;
			case Context.SubscriptionEventType.REMOVE:
				GLib.message("Context.SubscriptionEventType.REMOVE");
				pulse_update_sink_inputs();
				break;
			default:
				GLib.message("Sink input event not known.");
				break;
		}
	}

	private void pulse_context_state_callback (Context c)
	{
		switch (c.get_state ()) {
			case Context.State.READY:
				c.subscribe (PulseAudio.Context.SubscriptionMask.SINK_INPUT);
				c.set_subscribe_callback (context_events_cb);
				pulse_update_sink_inputs();
                                break;

                        case Context.State.FAILED:
                        case Context.State.TERMINATED:
				pulse_reconnect_soon();
                                break;

                        default:
                                break;
                }
        }

	private void pulse_disconnect()
	{
                if (_pulse_context != null) {
                        _pulse_context.disconnect ();
                        _pulse_context = null;
                }
	}

	private void pulse_reconnect_soon ()
	{
		if (_pulse_reconnect_timer == 0)
			_pulse_reconnect_timer = Timeout.add_seconds (2, pulse_reconnect_timeout);
	}

        private void pulse_reconnect_soon_cancel()
	{
                if (_pulse_reconnect_timer != 0) {
                        Source.remove(_pulse_reconnect_timer);
                        _pulse_reconnect_timer = 0;
                }
        }

        private bool pulse_reconnect_timeout ()
        {
                _pulse_reconnect_timer = 0;
                pulse_reconnect ();
                return false; // G_SOURCE_REMOVE
        }

        void pulse_reconnect ()
        {
		pulse_disconnect();

                var props = new Proplist ();
                props.sets (Proplist.PROP_APPLICATION_NAME, "Ubuntu Audio Settings");
                props.sets (Proplist.PROP_APPLICATION_ID, "com.canonical.settings.sound");
                props.sets (Proplist.PROP_APPLICATION_ICON_NAME, "multimedia-volume-control");
                props.sets (Proplist.PROP_APPLICATION_VERSION, "0.1");

                _pulse_context = new PulseAudio.Context (_pgloop.get_api(), null, props);
                _pulse_context.set_state_callback (pulse_context_state_callback);

                var server_string = Environment.get_variable("PULSE_SERVER");
                if (_pulse_context.connect(server_string, Context.Flags.NOFAIL, null) < 0)
                        warning( "pa_context_connect() failed: %s\n", PulseAudio.strerror(_pulse_context.errno()));
        }


	private void pulse_start()
	{
		pulse_reconnect();
	}

	private void pulse_stop()
	{
		pulse_reconnect_soon_cancel();
		pulse_disconnect();
		pulse_update_sink_inputs_cancel();
	}

#if 0
	/***
	****  Tracking multimedia volume via StreamRestore
	***/


	private async void connect_to_stream_restore()
	{
		_pconn = VolumeControlPulse.create_pulse_dbus_connection();
		GLib.message("_pconn is %p", (void*)_pconn);
		if (_pconn == null)
			return;

		_pconn.add_filter (pulse_dbus_filter);

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
#endif

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
		this.notify["multimedia-volume"].connect(() => {
			GLib.message("recalculating high-volume due to multimedia-volume change");
			this.update_high_volume();
		});
		this.notify["multimedia-active"].connect(() => {
			GLib.message("recalculating high-volume due to multimedia-active change");
			this.update_high_volume();
		});
		this.notify["headphones-active"].connect(() => {
			GLib.message("recalculating high-volume due to headphones-active change");
			this.update_high_volume();
		});
		notify["high-volume-approved"].connect(() => update_high_volume());
		update_high_volume();
	}
	private void update_high_volume() {
		GLib.message("calculating high volume... headphones_active %d high_volume_approved %d multimedia_active %d multimedia_volume %d is_loud %d", (int)headphones_active, (int)high_volume_approved, (int)multimedia_active, (int)multimedia_volume, (int)_options.is_loud_pulse(multimedia_volume));
		var new_high_volume = headphones_active
			&& !high_volume_approved
			&& multimedia_active
			&& _options.is_loud_pulse(multimedia_volume);
		GLib.message("so the new high_volume is %d, was %d", (int)new_high_volume, (int)high_volume);
		if (high_volume != new_high_volume) {
			debug("changing high_volume from %d to %d", (int)high_volume, (int)new_high_volume);
			high_volume = new_high_volume;
			if (high_volume && !active)
				show();
		}
	}

	/** HIGH VOLUME APPROVED PROPERTY **/

	private Settings _settings = new Settings ("com.canonical.indicator.sound");

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

	// NOTIFICATION

	private IndicatorSound.WarnNotification _notification = new IndicatorSound.WarnNotification();

	private PulseAudio.Volume _ok_volume = PulseAudio.Volume.INVALID;

        private uint _show_timeout = 0;

        private void stop_show_timeout() {
                if (_show_timeout != 0) {
                        Source.remove(_show_timeout);
                        _show_timeout = 0;
                }
        }

        private void show_soon() {
                const uint interval_msec = 200;
                if (_show_timeout == 0)
                        _show_timeout = Timeout.add(interval_msec, show_idle);
        }

        private bool show_idle() {
                _show_timeout = 0;
                this.show();
                return false; // Source.REMOVE;
        }

	private void show() {
		GLib.message("in show()");
		_ok_volume = multimedia_volume;
		GLib.message("calling notification.show");
		_notification.show();
		GLib.message("setting 'active' property to true");
		this.active = true;

		// lower the volume to just under the warning level
		GLib.message("setting multimedia volume to be just under the warning level");
		set_pulse_multimedia_volume.begin (_options.loud_volume()-1);
		GLib.message("leaving show()");
	}

	private void on_user_response(IndicatorSound.WarnNotification.Response response) {

		if (response == IndicatorSound.WarnNotification.Response.OK) {
			approve_high_volume();
			set_pulse_multimedia_volume.begin(_ok_volume);
		}

		_ok_volume = PulseAudio.Volume.INVALID;
		this.active = false;
	}
}
