/*
 * -*- Mode:Vala; indent-tabs-mode:t; tab-width:4; encoding:utf8 -*-
 *
 * Copyright 2015 Canonical Ltd.
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
 *      Charles Kerr <charles.kerr@canonical.com>
 */

using PulseAudio;

public abstract class VolumeWarning : Object
{
	// true if headphones are in use
	public bool headphones_active { get; set; default = false; }

	// true if the warning dialog is being shown
	public bool active { get; protected set; default = false; }

	// true if we're playing unapproved loud multimedia over headphones
	public bool high_volume { get; protected set; default = false; }

	public enum Key {
		VOLUME_UP,
		VOLUME_DOWN
	}

	public void user_keypress (Key key) {
		if ((key == Key.VOLUME_DOWN) && active) {
			_notification.close ();
			on_user_response (IndicatorSound.WarnNotification.Response.CANCEL);
		}
	}

	public VolumeWarning (IndicatorSound.Options options) {

		_options = options;

		init_high_volume ();
		init_approved ();

		_notification.user_responded.connect ((n, r) => on_user_response (r));
	}

	~VolumeWarning () {
		clear_timer (ref _approved_timer);
	}

	/***
	****
	***/

	// true if the user has approved high volumes recently
	protected bool approved { get; set; default = false; }

	// true if multimedia is currently playing
	protected bool multimedia_active { get; set; default = false; }

	/* Cached value of the multimedia volume reported by pulse.
	   Setting this only updates the cache -- to change the volume,
	   use sound_system_set_multimedia_volume.
	   NB: This PulseAudio.Volume is typed as uint to unconfuse valac. */
	protected uint multimedia_volume { get; set; default = PulseAudio.Volume.INVALID; }

	protected abstract void sound_system_set_multimedia_volume (PulseAudio.Volume volume);

	protected void clear_timer (ref uint timer) {
		if (timer != 0) {
			Source.remove (timer);
			timer = 0;
		}
	}

	private IndicatorSound.Options _options;

	/**
	*** HIGH VOLUME PROPERTY
	**/

	private void init_high_volume () {
		const string self_keys[] = {
			"multimedia-volume",
			"multimedia-active",
			"headphones-active",
			"high-volume-approved"
		};
		foreach (var key in self_keys)
			this.notify[key].connect (() => update_high_volume ());

		const string options_keys[] = {
			"loud-volume",
			"loud-warning-enabled"
		};
		foreach (var key in options_keys)
			_options.notify[key].connect (() => update_high_volume ());

		update_high_volume ();
	}

	private void update_high_volume () {

		var newval = _options.loud_warning_enabled
			&& headphones_active
			&& multimedia_active
			&& !approved
			&& (multimedia_volume != PulseAudio.Volume.INVALID)
			// from the sound specs:
			// "Whenever you increase volume,..., such that acoustic output would be MORE than 85 dB
			&& (multimedia_volume > _options.loud_volume);

		if (high_volume != newval) {
			debug ("changing high_volume from %d to %d", (int)high_volume, (int)newval);
			if (newval && !active)
				activate ();
			high_volume = newval;
		}
	}

	/**
	*** HIGH VOLUME APPROVED PROPERTY
	**/

	private Settings _settings = new Settings ("com.canonical.indicator.sound");
	private static const string TTL_KEY = "warning-volume-confirmation-ttl";
	private uint _approved_timer = 0;
	private int64 _approved_at = 0;
	private int64 _approved_ttl_usec = 0;

	private void approve_high_volume () {
		_approved_at = GLib.get_monotonic_time ();
		update_approved ();
		update_approved_timer ();
	}

	private void init_approved () {
		_settings.changed[TTL_KEY].connect (() => update_approved_cache ());
		update_approved_cache ();
	}
	private void update_approved_cache () {
		_approved_ttl_usec = _settings.get_int (TTL_KEY);
		_approved_ttl_usec *= 1000000;

		update_approved ();
		update_approved_timer ();
	}
	private void update_approved_timer () {

		clear_timer (ref _approved_timer);

		if (_approved_at == 0)
			return;

		int64 expires_at = _approved_at + _approved_ttl_usec;
		int64 now = GLib.get_monotonic_time ();
		if (expires_at > now) {
			var seconds_left = 1 + ((expires_at - now) / 1000000);
			_approved_timer = Timeout.add_seconds ((uint)seconds_left, () => {
				_approved_timer = 0;
				update_approved ();
				return Source.REMOVE;
			});
		}
	}
	private void update_approved () {
		var new_approved = calculate_approved ();
		if (approved != new_approved) {
			debug ("changing approved from %d to %d", (int)approved, (int)new_approved);
			approved = new_approved;
		}
	}
	private bool calculate_approved () {
		int64 now = GLib.get_monotonic_time ();
		return (_approved_at != 0)
			&& (_approved_at + _approved_ttl_usec >= now);
	}

	// NOTIFICATION

	private IndicatorSound.WarnNotification _notification = new IndicatorSound.WarnNotification ();
	private PulseAudio.Volume _ok_volume = PulseAudio.Volume.INVALID;

	protected virtual void preshow () {}

	private void activate () {
		preshow ();
		_ok_volume = multimedia_volume;

		_notification.show ();
		this.active = true;

		// lower the volume to just the warning level
		// from the sound specs:
		// "Whenever you increase volume,..., such that acoustic output would be MORE than 85 dB
		sound_system_set_multimedia_volume (_options.loud_volume);
	}

	private void on_user_response (IndicatorSound.WarnNotification.Response response) {

		this.active = false;

		if (response == IndicatorSound.WarnNotification.Response.OK) {
			approve_high_volume ();
			sound_system_set_multimedia_volume (_ok_volume);
		} else {
			this.cancel_pressed (this.volume_to_double(_options.loud_volume));
		}

		_ok_volume = PulseAudio.Volume.INVALID;
	}

	private static double volume_to_double (PulseAudio.Volume vol)
	{
		double tmp = (double)(vol - PulseAudio.Volume.MUTED);
		return tmp / (double)(PulseAudio.Volume.NORM - PulseAudio.Volume.MUTED);
	}

	public signal void cancel_pressed (double cancel_volume);
}
