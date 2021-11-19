/*
 * Copyright 2014 Canonical Ltd.
 * Copyright 2021 Robert Tari
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
 *      Robert Tari <robert@tari.in>
 */

[DBus (name = "com.lomiri.touch.AccountsService.Sound")]
public interface AccountsServiceSystemSoundSettings : Object {
    // properties
    public abstract bool silent_mode {owned get; set;}
}
