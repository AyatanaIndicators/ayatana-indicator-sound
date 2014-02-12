/*
 * Copyright © 2014 Canonical Ltd.
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

public class MediaPlayerMock: MediaPlayer {

	public override void activate () {
		debug("Mock activate");
	}
	public override void play_pause () {
		debug("Mock play_pause");
	}
	public override void next () {
		debug("Mock next");
	}
	public override void previous () {
		debug("Mock previous");
	}

	public override uint get_n_playlists() {
		debug("Mock get_n_playlists");
		return 0;
	}
	public override string get_playlist_id (int index) {
		debug("Mock get_playlist_id");
		return "";
	}
	public override string get_playlist_name (int index) {
		debug("Mock get_playlist_name");
		return "";
	}
	public override void activate_playlist_by_name (string playlist) {
		debug("Mock activate_playlist_by_name");
	}

}
