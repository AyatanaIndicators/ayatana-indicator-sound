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
	public override string stream { get { return ""; } }
	public override bool ready { get { return false; } set { } }
	public override bool active_mic { get { return false; } set { } }
	public override bool high_volume { get { return false; } }
	public override bool mute { get { return false; } }
	public override bool is_playing { get { return false; } }
	public override double volume { get { return 0.0; } set { } }
	public override double mic_volume { get { return 0.0; } set { } }

	public override void set_mute (bool mute) {
	
	}

	public VolumeControlMock() { 
		ready = true;
	}
}
