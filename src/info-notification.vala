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
		unowned string volume_label = "";

		switch (active_output) {
			case VolumeControl.ActiveOutput.SPEAKERS:
				volume_label = _("Speakers");
				break;
			case VolumeControl.ActiveOutput.HEADPHONES:
				volume_label = _("Headphones");
				break;
			case VolumeControl.ActiveOutput.BLUETOOTH_HEADPHONES:
				volume_label = _("Bluetooth headphones");
				break;
			case VolumeControl.ActiveOutput.BLUETOOTH_SPEAKER:
				volume_label = _("Bluetooth speaker");
				break;
			case VolumeControl.ActiveOutput.USB_SPEAKER:
				volume_label = _("Usb speaker");
				break;
			case VolumeControl.ActiveOutput.USB_HEADPHONES:
				volume_label = _("Usb headphones");
				break;
			case VolumeControl.ActiveOutput.HDMI_SPEAKER:
				volume_label = _("HDMI speaker");
				break;
			case VolumeControl.ActiveOutput.HDMI_HEADPHONES:
				volume_label = _("HDMI headphones");
				break;
		}

		return volume_label;
	}

	private static unowned string get_volume_notification_icon (VolumeControl.ActiveOutput active_output,
	                                                            double volume,
	                                                            bool is_high_volume) {
		unowned string icon = "";

		if (is_high_volume) {
			switch (active_output) {
				case VolumeControl.ActiveOutput.SPEAKERS:
				case VolumeControl.ActiveOutput.HEADPHONES:
				case VolumeControl.ActiveOutput.BLUETOOTH_HEADPHONES:
				case VolumeControl.ActiveOutput.BLUETOOTH_SPEAKER:
				case VolumeControl.ActiveOutput.USB_SPEAKER:
				case VolumeControl.ActiveOutput.USB_HEADPHONES:
				case VolumeControl.ActiveOutput.HDMI_SPEAKER:
				case VolumeControl.ActiveOutput.HDMI_HEADPHONES:
					icon = "audio-volume-high";
					break;
			}
		} else {
			icon = get_volume_icon (active_output, volume);
		}

		return icon;
	}

	private static unowned string get_volume_icon (VolumeControl.ActiveOutput active_output,
	                                               double volume)
	{
		unowned string icon = "";

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
					icon = "audio-volume-muted";
				else if (volume <= 0.3)
					icon = "audio-volume-low";
				else if (volume <= 0.7)
					icon = "audio-volume-medium";
				else
					icon = "audio-volume-high";
				break;
		}

		return icon;
	}
}

