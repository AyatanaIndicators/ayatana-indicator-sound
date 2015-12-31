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

	private static const string AMP_dB_KEY = "amplified-volume-decibels";
	private static const string NORMAL_dB_KEY = "normal-volume-decibels";
	private static const string ALLOW_AMP_KEY = "allow-amplified-volume";

        private void init_max_volume() {
                _settings.changed[NORMAL_dB_KEY].connect(() => update_max_volume());
                _settings.changed[AMP_dB_KEY].connect(() => update_max_volume());
                _shared_settings.changed[ALLOW_AMP_KEY].connect(() => update_max_volume());
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
                unowned string decibel_key = _shared_settings.get_boolean(ALLOW_AMP_KEY)
                        ? AMP_dB_KEY
                        : NORMAL_dB_KEY;
                var volume_dB = _settings.get_double(decibel_key);
                var volume_sw = PulseAudio.Volume.sw_from_dB (volume_dB);
                return VolumeControlPulse.volume_to_double (volume_sw);
        }


	/** LOUD VOLUME **/

	private static const string LOUD_ENABLED_KEY = "warning-volume-enabled";
	private static const string LOUD_DECIBEL_KEY = "warning-volume-decibels";

        private void init_loud_volume() {
                _settings.changed[LOUD_ENABLED_KEY].connect(() => update_loud_volume());
                _settings.changed[LOUD_DECIBEL_KEY].connect(() => update_loud_volume());
		update_loud_volume();
	}
	private void update_loud_volume() {

		var vol = PulseAudio.Volume.sw_from_dB (_settings.get_double (LOUD_DECIBEL_KEY));
		if (loud_volume != vol)
			loud_volume = vol;

		var enabled = _settings.get_boolean(LOUD_ENABLED_KEY);
		if (loud_warning_enabled != enabled)
			loud_warning_enabled = enabled;
        }
}
