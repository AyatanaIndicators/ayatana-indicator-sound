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

public class IndicatorSound.WarnNotification: Notification
{
	public enum Response {
		CANCEL,
		OK
	}

	public signal void user_responded(WarnNotification.Response response);

        protected override Notify.Notification create_notification() {
		var n = new Notify.Notification(_("Volume"), _("High volume can damage your hearing."), "audio-volume-high");
		n.set_hint ("x-canonical-non-shaped-icon", "true");
		n.set_hint ("x-canonical-snap-decisions", "true");
		n.set_hint ("x-canonical-private-affirmative-tint", "true");
		n.closed.connect((n) => {
			n.clear_actions();
		});
		return n;
	}

	public bool show() {
		close();

		if (!notify_server_supports("actions"))
			return false;

		_notification.clear_actions();
		_notification.add_action ("ok", _("OK"), (n, a) => {
			user_responded(Response.OK);
		});

		_notification.add_action ("cancel", _("Cancel"), (n, a) => {
			user_responded(Response.CANCEL);
		});

		show_();

		return true;
	}
}
