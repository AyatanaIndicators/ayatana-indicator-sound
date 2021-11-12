/*
 * Copyright (C) 2015 Canonical, Ltd.
 * Copyright (C) 2021 Robert Tari
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *      Xavi Garcia <xavi.garcia.mena@canonical.com>
 *      Robert Tari <robert@tari.in>
 */
#pragma once

namespace ayatana
{

namespace indicators
{

namespace testing
{
    constexpr const char ACCOUNTS_SERVICE[] = "org.freedesktop.Accounts";
    constexpr const char USER_PATH[] = "/org/freedesktop/Accounts/UserTest";
    constexpr const char ACCOUNTS_PATH[] = "/org/freedesktop/Accounts";
    constexpr const char ACCOUNTS_SOUND_INTERFACE[] = "com.lomiri.AccountsService.Sound";

} // namespace testing

} // namespace indicators

} // namespace ayatana
