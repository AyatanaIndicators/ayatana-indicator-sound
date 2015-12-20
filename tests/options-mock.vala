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
 *      Charles Kerr <charles.kerr@canonical.com>
 */

public class OptionsMock : IndicatorSound.Options
{
        // MAX VOLUME

        public void mock_set_max_volume(double val) { max_volume = val; }

        // LOUD

	private bool _loud_enabled = true;
	private PulseAudio.Volume _loud_volume;

        public override PulseAudio.Volume loud_volume() {
		return _loud_volume;
	}
        public override bool loud_volume_warning_enabled() {
		return _loud_enabled;
	}

	public void set_loud_enabled(bool v) {
		_loud_enabled = v;
		loud_changed();
	}
	public void set_loud_volume(PulseAudio.Volume v) {
		_loud_volume = v;
		loud_changed();
	}
}
