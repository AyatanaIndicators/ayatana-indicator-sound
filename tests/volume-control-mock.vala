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
	private bool _high_volume = false;
	public override bool high_volume { get { return _high_volume; } protected set { _high_volume = value; } }
	public void set_high_volume(bool b) { high_volume = b; }

	public string mock_stream { get; set; default = "multimedia"; }
	public override string stream { get { return mock_stream; } }
	public override bool ready { get; set; }
	public override bool active_mic { get; set; }
	public bool mock_mute { get; set; }
	public override bool mute { get { return mock_mute; } }
	public void mock_set_is_playing(bool b) { is_playing = b; }
	private VolumeControl.Volume _vol = new VolumeControl.Volume();
	public override VolumeControl.Volume volume { get { return _vol; } set { _vol = value; }}
	public override double mic_volume { get; set; }

	public override void set_mute (bool mute) {
	
	}

	public VolumeControlMock(IndicatorSound.Options options) {
		base(options);

		ready = true;
		this.notify["mock-stream"].connect(() => this.notify_property("stream"));
		this.notify["mock-high-volume"].connect(() => this.notify_property("high-volume"));
		this.notify["mock-mute"].connect(() => this.notify_property("mute"));
		this.notify["mock-is-playing"].connect(() => this.notify_property("is-playing"));
	}
}
