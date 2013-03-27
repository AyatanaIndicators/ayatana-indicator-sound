/*
 * Copyright 2013 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *      Alberto Ruiz <alberto.ruiz@canonical.com>
 */

using PulseAudio;

[CCode(cname="pa_cvolume_set", cheader_filename = "pulse/volume.h")]
extern unowned PulseAudio.CVolume? vol_set (PulseAudio.CVolume? cv, uint channels, PulseAudio.Volume v);

public class VolumeControl : Object
{
	/* this is static to ensure it being freed after @context (loop does not have ref counting) */
	private static PulseAudio.GLibMainLoop loop;

	private PulseAudio.Context context;
	private bool   _mute = true;
	private double _volume = 0.0;

	public signal void ready ();
	public signal void volume_changed (double v);

	public VolumeControl ()
	{
		if (loop == null)
			loop = new PulseAudio.GLibMainLoop ();

		var props = new Proplist ();
		props.sets (Proplist.PROP_APPLICATION_NAME, "Ubuntu Audio Settings");
		props.sets (Proplist.PROP_APPLICATION_ID, "com.canonical.settings.sound");
		props.sets (Proplist.PROP_APPLICATION_ICON_NAME, "multimedia-volume-control");
		props.sets (Proplist.PROP_APPLICATION_VERSION, "0.1");

		context = new PulseAudio.Context (loop.get_api(), null, props);

		context.set_state_callback (notify_cb);

		if (context.connect(null, Context.Flags.NOFAIL, null) < 0)
		{
			warning( "pa_context_connect() failed: %s\n", PulseAudio.strerror(context.errno()));
			return;
		}
	}

	/* PulseAudio logic*/
	private void context_events_cb (Context c, Context.SubscriptionEventType t, uint32 index)
	{
		if ((t & Context.SubscriptionEventType.FACILITY_MASK) == Context.SubscriptionEventType.SINK)
		{
			get_properties ();
		}
	}

	private void sink_info_cb_for_props (Context c, SinkInfo? i, int eol)
	{
		if (i == null)
			return;

		if (_mute != (bool)i.mute)
		{
			_mute = (bool)i.mute;
			this.notify_property ("mute");
		}

		if (_volume != volume_to_double (i.volume.values[0]))
		{
			_volume = volume_to_double (i.volume.values[0]);
			volume_changed (_volume);
		}
	}

	private void server_info_cb_for_props (Context c, ServerInfo? i)
	{
		if (i == null)
			return;
		context.get_sink_info_by_name (i.default_sink_name, sink_info_cb_for_props);
	}

	private void get_properties ()
	{
		context.get_server_info (server_info_cb_for_props);
	}

	private void notify_cb (Context c)
	{
		if (c.get_state () == Context.State.READY)
		{
			c.subscribe (PulseAudio.Context.SubscriptionMask.SINK);
			c.set_subscribe_callback (context_events_cb);
			get_properties ();
			ready ();
		}
	}

	/* Mute operations */
	public void set_mute (bool mute)
	{
		if (context.get_state () != Context.State.READY)
		{
			warning ("Could not mute: PulseAudio server connection is not ready.");
			return;
		}

		context.get_sink_info_list ((context, sink, eol) => {
			if (sink != null)
				context.set_sink_mute_by_index (sink.index, mute, null);
		});
	}

	public void toggle_mute ()
	{
		this.set_mute (!this._mute);
	}

	public bool mute
	{
		get
		{
			return this._mute;
		}
	}

	/* Volume operations */
	private static PulseAudio.Volume double_to_volume (double vol)
	{
		double tmp = (double)(PulseAudio.Volume.NORM - PulseAudio.Volume.MUTED) * vol;
		return (PulseAudio.Volume)tmp + PulseAudio.Volume.MUTED;
	}

	private static double volume_to_double (PulseAudio.Volume vol)
	{
		double tmp = (double)(vol - PulseAudio.Volume.MUTED);
		return tmp / (double)(PulseAudio.Volume.NORM - PulseAudio.Volume.MUTED);
	}

	private void set_volume_success_cb (Context c, int success)
	{
		if ((bool)success)
			volume_changed (_volume);
	}

	private void sink_info_set_volume_cb (Context c, SinkInfo? i, int eol)
	{
		if (i == null)
			return;

		unowned CVolume cvol = vol_set (i.volume, 1, double_to_volume (_volume));
		c.set_sink_volume_by_index (i.index, cvol, set_volume_success_cb);
	}

	private void server_info_cb_for_set_volume (Context c, ServerInfo? i)
	{
		if (i == null)
		{
			warning ("Could not get PulseAudio server info");
			return;
		}

		context.get_sink_info_by_name (i.default_sink_name, sink_info_set_volume_cb);
	}

	public void set_volume (double volume)
	{
		if (context.get_state () != Context.State.READY)
		{
			warning ("Could not change volume: PulseAudio server connection is not ready.");
			return;
		}
		_volume = volume;

		context.get_server_info (server_info_cb_for_set_volume);
	}

	public double get_volume ()
	{
		return _volume;
	}
}
