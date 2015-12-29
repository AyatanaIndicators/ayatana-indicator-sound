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

public class VolumeWarningMock : VolumeWarning
{
	public void set_high_volume(bool b) { high_volume = b; }

	public VolumeWarningMock(IndicatorSound.Options options) {
		base(options);
	}

        protected override void sound_system_set_multimedia_volume(PulseAudio.Volume volume) {
		GLib.message("volume-warning-mock setting multimedia volume to %d", (int)volume);
	}
}
