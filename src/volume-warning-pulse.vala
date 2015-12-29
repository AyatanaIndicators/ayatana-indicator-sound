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

public class VolumeWarningPulse : VolumeWarning
{
	public VolumeWarningPulse (IndicatorSound.Options options,
	                           PulseAudio.GLibMainLoop pgloop) {
		base(options);

		_pgloop = pgloop;
		pulse_reconnect();
	}

	~VolumeWarningPulse () {
		clear_timer(ref _pulse_reconnect_timer);
		clear_timer(ref _update_sink_timer);
		clear_timer(ref _update_sink_inputs_timer);
		pulse_disconnect();
	}

	protected override void sound_system_set_multimedia_volume (PulseAudio.Volume volume) {
		pulse_set_sink_volume(volume);
	}

	/***
	****  PulseAudio: Tracking the active multimedia sink input
	***/

	private unowned PulseAudio.GLibMainLoop _pgloop = null;
	private PulseAudio.Context _pulse_context = null;
	private uint _pulse_reconnect_timer = 0;
	private uint _update_sink_timer = 0;
	private uint _update_sink_inputs_timer = 0;
        private GenericSet<uint32> _pending_sink_inputs = new GenericSet<uint32>(direct_hash, direct_equal);


	private uint32 _warning_sink_index          = PulseAudio.INVALID_INDEX;
	private uint32 _multimedia_sink_index       = PulseAudio.INVALID_INDEX;
	private uint32 _multimedia_sink_input_index = PulseAudio.INVALID_INDEX;

	private unowned PulseAudio.CVolume _multimedia_cvolume;

	/***/

	private void update_multimedia_volume () {

		GLib.return_if_fail(_pulse_context != null);
		GLib.return_if_fail(_multimedia_sink_index != PulseAudio.INVALID_INDEX);

		_pulse_context.get_sink_info_by_index(_multimedia_sink_index, (c,i) => {
			if (i != null) {
				GLib.message("setting multimedia_volume to %s", i.volume.to_string());
				_multimedia_cvolume = i.volume;
				multimedia_volume = i.volume.max();
			}
		});
	}

	private void update_multimedia_volume_soon () {

		GLib.message("updating multimedia volume soon");
		if (_update_sink_timer == 0) {
			_update_sink_timer = Timeout.add_seconds (1, () => {
				_update_sink_timer = 0;
				update_multimedia_volume ();
				return Source.REMOVE;
			});
		}
	}

	private void set_multimedia_sink_index(uint32 index) {

		if (index == PulseAudio.INVALID_INDEX) {
			_multimedia_sink_index = PulseAudio.INVALID_INDEX;
			multimedia_volume = PulseAudio.Volume.INVALID;
		} else if (_multimedia_sink_index != index) {
			_multimedia_sink_index = index;
			multimedia_volume = PulseAudio.Volume.INVALID;
			update_multimedia_volume_soon();
		}
	}

	/***/

	private bool is_active_multimedia (SinkInputInfo i) {

		if (i.corked != 0)
			return false;

		var key = PulseAudio.Proplist.PROP_MEDIA_ROLE;
		var media_role = i.proplist.gets(key);
		if (media_role != "multimedia")
			return false;

		return true;
	}

	private void on_sink_input_info (Context c, SinkInputInfo? i, int eol) {

		if (i == null)
			return;

		if (is_active_multimedia (i)) {
			GLib.message ("on_sink_input_info() setting multimedia sink input index to %d, sink index to %d", (int)i.index, (int)i.sink);
			_multimedia_sink_input_index = i.index;
			set_multimedia_sink_index (i.sink);
			multimedia_active = true;
		}
		else if (i.index == _multimedia_sink_input_index) {
			_multimedia_sink_input_index = PulseAudio.INVALID_INDEX;
			set_multimedia_sink_index (PulseAudio.INVALID_INDEX);
			multimedia_active = false;
		}
	}

	private void update_all_sink_inputs () {
		_pulse_context.get_sink_input_info_list (on_sink_input_info);
	}
	private void update_sink_input (uint32 index) {
		_pulse_context.get_sink_input_info (index, on_sink_input_info);
	}

	private void update_sink_input_soon (uint32 index) {

		GLib.message("adding %d to queue", (int)index);
		_pending_sink_inputs.add(index);

		if (_update_sink_inputs_timer == 0) {
			_update_sink_inputs_timer = Timeout.add_seconds (1, () => {
				_pending_sink_inputs.foreach((i) => {
					GLib.message("flushing input sink queue: index %d", (int)i);
					update_sink_input(i);
				});
				_pending_sink_inputs.remove_all();
				_update_sink_inputs_timer = 0;
				return Source.REMOVE;
			});
		}
	}

        private void context_events_cb (Context c, Context.SubscriptionEventType t, uint32 index) {
		switch (t & Context.SubscriptionEventType.FACILITY_MASK)
		{
			case Context.SubscriptionEventType.SINK:
				if ((index == _multimedia_sink_index) && (index != PulseAudio.INVALID_INDEX))
					update_multimedia_volume_soon();
				break;

			case Context.SubscriptionEventType.SINK_INPUT:
				switch (t & Context.SubscriptionEventType.TYPE_MASK)
				{
					case Context.SubscriptionEventType.NEW:
					case Context.SubscriptionEventType.CHANGE:
						GLib.message ("-> Context.SubscriptionEventType.CHANGE or NEW");
						update_sink_input_soon(index);
						break;
					case Context.SubscriptionEventType.REMOVE:
						GLib.message ("-> Context.SubscriptionEventType.REMOVE");
						update_all_sink_inputs ();
						break;
					default:
						GLib.debug ("Sink input event not known.");
						break;
				}
                                break;

			default:
				break;
		}
	}

	private void pulse_context_state_callback (Context c) {
		switch (c.get_state ()) {
			case Context.State.READY:
				c.set_subscribe_callback (context_events_cb);
				c.subscribe (PulseAudio.Context.SubscriptionMask.SINK |
				             PulseAudio.Context.SubscriptionMask.SINK_INPUT);
				update_all_sink_inputs ();
				break;

			case Context.State.FAILED:
			case Context.State.TERMINATED:
				pulse_reconnect_soon ();
				break;

			default:
				break;
		}
	}

	private void pulse_disconnect () {
		if (_pulse_context != null) {
			_pulse_context.disconnect ();
			_pulse_context = null;
		}
	}

	private void pulse_reconnect_soon () {
		if (_pulse_reconnect_timer == 0)
			_pulse_reconnect_timer = Timeout.add_seconds (2, pulse_reconnect_timeout);
	}

	private bool pulse_reconnect_timeout () {
		_pulse_reconnect_timer = 0;
		pulse_reconnect ();
		return Source.REMOVE;
	}

	void pulse_reconnect () {
		pulse_disconnect();

		var props = new Proplist ();
		props.sets (Proplist.PROP_APPLICATION_NAME, "Ubuntu Audio Settings");
		props.sets (Proplist.PROP_APPLICATION_ID, "com.canonical.settings.sound");
		props.sets (Proplist.PROP_APPLICATION_ICON_NAME, "multimedia-volume-control");
		props.sets (Proplist.PROP_APPLICATION_VERSION, "0.1");

		_pulse_context = new PulseAudio.Context (_pgloop.get_api(), null, props);
		_pulse_context.set_state_callback (pulse_context_state_callback);

		var server_string = Environment.get_variable ("PULSE_SERVER");
		if (_pulse_context.connect (server_string, Context.Flags.NOFAIL, null) < 0)
			GLib.warning ("pa_context_connect() failed: %s\n", PulseAudio.strerror(_pulse_context.errno()));
	}

	void pulse_set_sink_volume (PulseAudio.Volume volume) {
		var index = _warning_sink_index;

		GLib.return_if_fail(_pulse_context != null);
		GLib.return_if_fail(index != PulseAudio.INVALID_INDEX);
		GLib.return_if_fail(volume != PulseAudio.Volume.INVALID);

		unowned CVolume cvol = CVolume ();
		cvol.set (_multimedia_cvolume.channels, volume);
		GLib.message ("setting multimedia volume to %s", cvol.to_string());
		_pulse_context.set_sink_volume_by_index (index, cvol);
	}

        protected override void preshow () {
                _warning_sink_index = _multimedia_sink_index;
        }
}
