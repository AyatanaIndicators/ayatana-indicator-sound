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

using PulseAudio;

public class IndicatorSound.OptionsGSettings : Options
{
	public OptionsGSettings() {
        	init_max_volume();
        	init_loud_volume();
	}

	~OptionsGSettings() {
	}

        private Settings _settings = new Settings ("com.canonical.indicator.sound");
        private Settings _shared_settings = new Settings ("com.ubuntu.sound");

        /** MAX VOLUME PROPERTY **/

        private void init_max_volume() {
                _settings.changed["normal-volume-decibels"].connect(() => update_max_volume());
                _settings.changed["amplified-volume-decibels"].connect(() => update_max_volume());
                _shared_settings.changed["allow-amplified-volume"].connect(() => update_max_volume());
                update_max_volume();
        }
        private void update_max_volume () {
                set_max_volume_(calculate_max_volume());
        }
        protected void set_max_volume_ (double vol) {
                if (max_volume != vol) {
                        debug("changing max_volume from %f to %f", this.max_volume, vol);
                        max_volume = vol;
                }
        }
        private double calculate_max_volume () {
                unowned string decibel_key = _shared_settings.get_boolean("allow-amplified-volume")
                        ? "amplified-volume-decibels"
                        : "normal-volume-decibels";
                var volume_dB = _settings.get_double(decibel_key);
                var volume_sw = PulseAudio.Volume.sw_from_dB (volume_dB);
                return VolumeControlPulse.volume_to_double (volume_sw);
        }


	/** LOUD VOLUME **/

	private PulseAudio.Volume _loud_volume;

	public override PulseAudio.Volume loud_volume() {
		return _loud_volume;
	}

	private bool _loud_volume_warning_enabled;

	public override bool loud_volume_warning_enabled() {
		return _loud_volume_warning_enabled;
	}

	private string loud_enabled_key = "warning-volume-enabled";
	private string loud_decibel_key = "warning-volume-decibels";

        private void init_loud_volume() {
                _settings.changed[loud_enabled_key].connect(() => update_loud_volume());
                _settings.changed[loud_decibel_key].connect(() => update_loud_volume());
		update_loud_volume();
	}
	private void update_loud_volume() {
                var changed = false;
		var vol = PulseAudio.Volume.sw_from_dB (_settings.get_double (loud_decibel_key));
		var enabled = _settings.get_boolean(loud_enabled_key);

                if (_loud_volume != vol) {
                        debug("updating loud_volume_sw to %d", (int)vol);
                        _loud_volume = vol;
                        changed = true;
                }
                if (_loud_volume_warning_enabled != enabled) {
                        debug("updating loud_volume_warning_enabled to %d", (int)enabled);
                        _loud_volume_warning_enabled = enabled;
                        changed = true;
                }

                if (changed)
                        loud_changed();
        }
}
