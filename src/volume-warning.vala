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

	public VolumeWarning (IndicatorSound.Options options) {
		_options = options;

		init_all_properties();

		connect_to_stream_restore.begin();

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
	}

	/***
	****
	***/

	// FIXME: what to do with this now?
	private bool   _ignore_warning_this_time = false;

	/* Used by the pulseaudio stream restore extension */
	private DBusConnection _pconn;

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
		stop_high_volume_approved_timer();
	}

	/***
	****  Tracking multimedia volume via StreamRestore
	***/

	private DBusMessage pulse_dbus_filter (DBusConnection connection, owned DBusMessage message, bool incoming)
	{
		if (message.get_message_type() == DBusMessageType.SIGNAL)
		{
			var member = message.get_member();
			var path = message.get_path();
			GLib.message ("path [%s] member [%s] _multimedia_objp [%s]", path, member, _multimedia_objp);

			if ((member == "VolumeUpdated") && (path == _multimedia_objp))
			{
				Variant body = message.get_body ();
				Variant varray = body.get_child_value (0);
				uint32 type = 0, lvolume = 0;
				VariantIter iter = varray.iterator ();
				iter.next ("(uu)", &type, &lvolume);
				if (multimedia_volume != lvolume) {
					GLib.message("setting multimedia_volume to %d from VolumeUpdated signal %s", (int)lvolume, body.print(true));
					multimedia_volume = lvolume;
				}
			}
			else if (((member == "NewEntry") || (member == "EntryRemoved")) && (path == "/org/pulseaudio/stream_restore1"))
			{
				update_multimedia_objp();
			}
		}

		return message;
	}

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

	private void show() {
		_ok_volume = multimedia_volume;
		_notification.show();
		this.active = true;

		// lower the volume to just under the warning level
		set_pulse_multimedia_volume.begin (_options.loud_volume()-1);
	}

	private void on_user_response(IndicatorSound.WarnNotification.Response response) {
		_notification.close();

		if (response == IndicatorSound.WarnNotification.Response.OK) {
			approve_high_volume();
			set_pulse_multimedia_volume.begin(_ok_volume);
		}

		_ok_volume = PulseAudio.Volume.INVALID;

		this.active = false;
	}
}
