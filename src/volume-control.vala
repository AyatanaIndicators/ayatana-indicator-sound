/*
 * -*- Mode:Vala; indent-tabs-mode:t; tab-width:4; encoding:utf8 -*-
 * Copyright © 2015 Canonical Ltd.
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

public abstract class VolumeControl : Object
{
	public enum VolumeReasons {
		PULSE_CHANGE,
		ACCOUNTS_SERVICE_SET,
		DEVICE_OUTPUT_CHANGE,
		USER_KEYPRESS,
		VOLUME_STREAM_CHANGE
	}

	public enum ActiveOutput {
		SPEAKERS,
		HEADPHONES,
		BLUETOOTH_HEADPHONES,
		BLUETOOTH_SPEAKER,
		USB_SPEAKER,
		USB_HEADPHONES,
		HDMI_SPEAKER,
		HDMI_HEADPHONES,
		CALL_MODE
	}

	public class Volume : Object {
		public double volume;
		public VolumeReasons reason;
	}

	protected IndicatorSound.Options _options = null;

	public VolumeControl(IndicatorSound.Options options) {
		_options = options;
	}

	public virtual string stream { get { return ""; } }
	public bool ready { get; protected set; default = false; }
	public virtual bool active_mic { get { return false; } set { } }
	public virtual bool high_volume { get { return false; } protected set { } }
	public virtual bool mute { get { return false; } }
	public bool is_playing { get; protected set; default = false; }
	public virtual VolumeControl.ActiveOutput active_output { get { return VolumeControl.ActiveOutput.SPEAKERS; } }
	private Volume _volume;
	private double _pre_clamp_volume;
	public virtual Volume volume { get { return _volume; } set { } }
	public virtual double mic_volume { get { return 0.0; } set { } }

	public virtual void clamp_to_high_volume() { }
	public virtual void set_warning_volume() { }

	public abstract void set_mute (bool mute);

	public void set_volume_clamp (double unclamped, VolumeControl.VolumeReasons reason) {
		var v = new VolumeControl.Volume();
		v.volume = unclamped.clamp (0.0, _options.max_volume);
		v.reason = reason;
		this.volume = v;
		_pre_clamp_volume = unclamped;
	}

	public double get_pre_clamped_volume () {
		return _pre_clamp_volume;
	}

	public signal void active_output_changed (VolumeControl.ActiveOutput active_output);
}
