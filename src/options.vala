/*
 * -*- Mode:Vala; indent-tabs-mode:t; tab-width:4; encoding:utf8 -*-
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

public abstract class IndicatorSound.Options : Object
{
	// MAX VOLUME

	public double max_volume { get; protected set; default = 1.0; }

	// LOUD

	public signal void loud_changed();
	public abstract PulseAudio.Volume loud_volume();
	public abstract bool loud_volume_warning_enabled();

	public bool is_loud_pulse (PulseAudio.Volume volume) {
		return loud_volume_warning_enabled() && (volume >= loud_volume());
	}

	public bool is_loud (VolumeControl.Volume volume) {
		return is_loud_pulse(VolumeControlPulse.double_to_volume(volume.volume));
	}
}
