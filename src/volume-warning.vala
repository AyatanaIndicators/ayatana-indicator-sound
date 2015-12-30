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
			_notification.close();
			on_user_response(IndicatorSound.WarnNotification.Response.CANCEL);
		}
	}

	public VolumeWarning (IndicatorSound.Options options) {

		_options = options;

		init_all_properties();

		_notification = new IndicatorSound.WarnNotification ();
		_notification.user_responded.connect((n, r) => on_user_response(r));
	}

	~VolumeWarning () {
		stop_all_timers();
	}

	/***
	****
	***/

	// true if the user has approved high volumes recently
	protected bool high_volume_approved { get; set; default = false; }

	// true if multimedia is currently playing
	protected bool multimedia_active { get; set; default = false; }

	/* Cached value of the multimedia volume reported by pulse.
	   Setting this only updates the cache -- to change the volume,
	   use sound_system_set_multimedia_volume().
	   NB: This PulseAudio.Volume is typed as uint to unconfuse valac. */
	protected uint multimedia_volume { get; set; default = PulseAudio.Volume.INVALID; }

	protected abstract void sound_system_set_multimedia_volume(PulseAudio.Volume volume);

	protected void clear_timer (ref uint timer) {
		if (timer != 0) {
			Source.remove (timer);
			timer = 0;
		}
	}

	/***
	****
	***/

	private IndicatorSound.Options _options;

	private void init_all_properties()
	{
		init_high_volume();
		init_high_volume_approved();
	}

	private void stop_all_timers()
	{
		stop_high_volume_approved_timer();
	}

	/**
	*** HIGH VOLUME PROPERTY
	**/

	private void init_high_volume () {
		this.notify["multimedia-volume"].connect(() => update_high_volume());
		this.notify["multimedia-active"].connect(() => update_high_volume());
		this.notify["headphones-active"].connect(() => update_high_volume());
		this.notify["high-volume-approved"].connect(() => update_high_volume());
		_options.notify["loud-volume"].connect(() => update_high_volume());
		_options.notify["loud-warning-enabled"].connect(() => update_high_volume());
		update_high_volume();
	}

	private void update_high_volume () {

		var newval = _options.loud_warning_enabled
			&& headphones_active
			&& multimedia_active
			&& !high_volume_approved
			&& (multimedia_volume != PulseAudio.Volume.INVALID)
			&& (multimedia_volume >= _options.loud_volume);

		if (high_volume != newval) {
			debug("changing high_volume from %d to %d", (int)high_volume, (int)newval);
			if (newval && !active)
				show();
			high_volume = newval;
		}
	}

	/**
	*** HIGH VOLUME APPROVED PROPERTY
	**/

	private Settings _settings = new Settings ("com.canonical.indicator.sound");
	private uint _high_volume_approved_timer = 0;
	private int64 _high_volume_approved_at = 0;
	private int64 _high_volume_approved_ttl_usec = 0;

	private void approve_high_volume() {
		_high_volume_approved_at = GLib.get_monotonic_time();
		update_high_volume_approved();
		update_high_volume_approved_timer();
	}

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
		clear_timer(ref _high_volume_approved_timer);
	}
	private bool on_high_volume_approved_timer() {
		_high_volume_approved_timer = 0;
		update_high_volume_approved();
		return Source.REMOVE;
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

	protected virtual void preshow() {}

	private void show() {
		preshow();
		_ok_volume = multimedia_volume;

		_notification.show();
		this.active = true;

		// lower the volume to just under the warning level
		sound_system_set_multimedia_volume (_options.loud_volume-1);
	}

	private void on_user_response(IndicatorSound.WarnNotification.Response response) {

		this.active = false;

		if (response == IndicatorSound.WarnNotification.Response.OK) {
			approve_high_volume();
			sound_system_set_multimedia_volume(_ok_volume);
		}

		_ok_volume = PulseAudio.Volume.INVALID;
	}
}
