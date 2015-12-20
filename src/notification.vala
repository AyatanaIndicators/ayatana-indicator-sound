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

	public Notification() {
		BusWatcher.watch_namespace (GLib.BusType.SESSION,
		                            "org.freedesktop.Notifications",
		                            () => { debug("Notifications name appeared"); },
		                            () => { debug("Notifications name vanshed"); _server_caps = null; });

		_notification = create_notification();
		_notification.closed.connect((n) => {
			visible = false;
		});
	}

	protected abstract Notify.Notification create_notification();

	~Notification() {
		close();
	}

	protected void show_() {
		try {
			_notification.show ();
			message("after calling show, n.id is %d", (int)_notification.id);
			visible = true;
		} catch (GLib.Error e) {
			warning ("Unable to show notification: %s", e.message);
		}
	}

	public void close() {
		var n = _notification;

		return_if_fail (n != null);

		message("closing id %d", n.id);
		if (n.id != 0) {
			try {
				n.close();
			} catch (GLib.Error e) {
				warning("Unable to close notification: %s", e.message);
			}
		}
	}

	protected bool notify_server_supports(string cap) {
		if (_server_caps == null) {
			message("getting server caps");
			_server_caps = Notify.get_server_caps();
		}

		var ret = _server_caps.find_custom(cap, strcmp) != null;
		message("%s --> %d", cap, (int)ret);
		return ret;
	}

	protected Notify.Notification _notification = null;
	private List<string> _server_caps = null;

}
