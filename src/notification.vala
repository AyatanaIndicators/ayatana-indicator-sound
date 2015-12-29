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

public abstract class IndicatorSound.Notification: Object
{
	public bool visible { get; protected set; default = false; }

	public Notification () {
		BusWatcher.watch_namespace (GLib.BusType.SESSION,
		                            "org.freedesktop.Notifications",
		                            () => { debug("Notifications name appeared"); },
		                            () => { debug("Notifications name vanshed"); _server_caps = null; });

		_notification = create_notification ();
		_notification.closed.connect((n) => {
			visible = false;
		});
	}

	public void close() {
		var n = _notification;

		return_if_fail (n != null);

		if (n.id != 0) {
			try {
				n.close();
			} catch (GLib.Error e) {
				GLib.warning("Unable to close notification: %s", e.message);
			}
		}
	}

	~Notification () {
		close ();
	}

	protected abstract Notify.Notification create_notification ();

	protected void show_notification () {
		try {
			_notification.show ();
			visible = true;
		} catch (GLib.Error e) {
			GLib.warning ("Unable to show notification: %s", e.message);
		}
	}

	protected bool notify_server_supports (string cap) {
		if (_server_caps == null)
			_server_caps = Notify.get_server_caps();

		return _server_caps.find_custom(cap, strcmp) != null;
	}

	protected Notify.Notification _notification = null;

	private static List<string> _server_caps = null;
}
