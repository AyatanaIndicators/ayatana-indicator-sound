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

public class VolumeControlMock : VolumeControl
{
	public string mock_stream { get; set; default = "multimedia"; }
	public override string stream { get { return mock_stream; } }
	public override bool ready { get; set; }
	public override bool active_mic { get; set; }
	public bool mock_high_volume { get; set; }
	public override bool high_volume { get { return mock_high_volume; } }
	public bool mock_mute { get; set; }
	public override bool mute { get { return mock_mute; } }
	public bool mock_is_playing { get; set; }
	public override bool is_playing { get { return mock_is_playing; } }
	public override double volume { get; set; }
	public override double mic_volume { get; set; }

	public override void set_mute (bool mute) {
	
	}

	public VolumeControlMock() { 
		ready = true;
	}
}
