/*
 * Copyright 2014 © Canonical Ltd.
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


public class AccountsServiceUser : Object {
	Act.UserManager accounts_manager = Act.UserManager.get_default();
	Act.User? user = null;

	public AccountsServiceUser () {
		user = accounts_manager.get_user(GLib.Environment.get_user_name());

	}









}
