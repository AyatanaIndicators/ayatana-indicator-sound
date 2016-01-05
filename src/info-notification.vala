/*
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

using Notify;

public class IndicatorSound.InfoNotification: Notification
{
	protected override Notify.Notification create_notification () {
		return new Notify.Notification (_("Volume"), "", "audio-volume-muted");
	}

	public void show (VolumeControl.ActiveOutput active_output,
	                  double volume,
	                  bool is_high_volume) {
		if (!notify_server_supports ("x-canonical-private-synchronous"))
			return;

		/* Determine Label */
	        unowned string volume_label = get_notification_label (active_output);

		/* Choose an icon */
	 	unowned string icon = get_volume_notification_icon (active_output, volume, is_high_volume);

		/* Reset the notification */
		var n = _notification;
		n.update (_("Volume"), volume_label, icon);
		n.clear_hints();
		n.set_hint ("x-canonical-non-shaped-icon", "true");
		n.set_hint ("x-canonical-private-synchronous", "true");
		n.set_hint ("x-canonical-value-bar-tint", is_high_volume ? "true" : "false");
		n.set_hint ("value", ((int32)((volume * 100.0) + 0.5)).clamp(0, 100));
		show_notification ();
	}

	private static unowned string get_notification_label (VolumeControl.ActiveOutput active_output) {

		switch (active_output) {
			case VolumeControl.ActiveOutput.SPEAKERS:
				return _("Speakers");
			case VolumeControl.ActiveOutput.HEADPHONES:
				return _("Headphones");
			case VolumeControl.ActiveOutput.BLUETOOTH_HEADPHONES:
				return _("Bluetooth headphones");
			case VolumeControl.ActiveOutput.BLUETOOTH_SPEAKER:
				return _("Bluetooth speaker");
			case VolumeControl.ActiveOutput.USB_SPEAKER:
				return _("Usb speaker");
			case VolumeControl.ActiveOutput.USB_HEADPHONES:
				return _("Usb headphones");
			case VolumeControl.ActiveOutput.HDMI_SPEAKER:
				return _("HDMI speaker");
			case VolumeControl.ActiveOutput.HDMI_HEADPHONES:
				return _("HDMI headphones");
			default:
				return "";
		}
	}

	private static unowned string get_volume_notification_icon (VolumeControl.ActiveOutput active_output,
	                                                            double volume,
	                                                            bool is_high_volume) {

		if (!is_high_volume)
			return get_volume_icon (active_output, volume);

		switch (active_output) {
			case VolumeControl.ActiveOutput.SPEAKERS:
			case VolumeControl.ActiveOutput.HEADPHONES:
			case VolumeControl.ActiveOutput.BLUETOOTH_HEADPHONES:
			case VolumeControl.ActiveOutput.BLUETOOTH_SPEAKER:
			case VolumeControl.ActiveOutput.USB_SPEAKER:
			case VolumeControl.ActiveOutput.USB_HEADPHONES:
			case VolumeControl.ActiveOutput.HDMI_SPEAKER:
			case VolumeControl.ActiveOutput.HDMI_HEADPHONES:
				return "audio-volume-high";

			default:
				return "";
		}
	}

	private static unowned string get_volume_icon (VolumeControl.ActiveOutput active_output,
	                                               double volume)
	{
		switch (active_output) {
			case VolumeControl.ActiveOutput.SPEAKERS:
			case VolumeControl.ActiveOutput.HEADPHONES:
			case VolumeControl.ActiveOutput.BLUETOOTH_HEADPHONES:
			case VolumeControl.ActiveOutput.BLUETOOTH_SPEAKER:
			case VolumeControl.ActiveOutput.USB_SPEAKER:
			case VolumeControl.ActiveOutput.USB_HEADPHONES:
			case VolumeControl.ActiveOutput.HDMI_SPEAKER:
			case VolumeControl.ActiveOutput.HDMI_HEADPHONES:
				if (volume <= 0.0)
					return "audio-volume-muted";
				if (volume <= 0.3)
					return "audio-volume-low";
				if (volume <= 0.7)
					return "audio-volume-medium";
				return "audio-volume-high";

			default:
				return "";
		}
	}
}

