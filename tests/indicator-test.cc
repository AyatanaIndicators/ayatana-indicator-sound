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

#include <gtest/gtest.h>
#include <gio/gio.h>

#include "indicator-fixture.h"
#include "accounts-service-mock.h"
#include "notifications-mock.h"

class IndicatorTest : public IndicatorFixture
{
protected:
	IndicatorTest (void) :
		IndicatorFixture(INDICATOR_SOUND_SERVICE_BINARY, "com.canonical.indicator.sound")
	{
	}

	std::shared_ptr<AccountsServiceMock> as;
	std::shared_ptr<NotificationsMock> notification;

	virtual void SetUp() override
	{
		//addMock(buildBustleMock("indicator-test-session.bustle", DBUS_TEST_SERVICE_BUS_SESSION));
		//addMock(buildBustleMock("indicator-test-system.bustle", DBUS_TEST_SERVICE_BUS_SYSTEM));
		g_setenv("LD_PRELOAD", PA_MOCK_LIB, TRUE);

		g_setenv("GSETTINGS_SCHEMA_DIR", SCHEMA_DIR, TRUE);
		g_setenv("GSETTINGS_BACKEND", "memory", TRUE);

		as = std::make_shared<AccountsServiceMock>();
		addMock(*as);

		notification = std::make_shared<NotificationsMock>();
		addMock(*notification);

		IndicatorFixture::SetUp();
	}

	virtual void TearDown() override
	{
		as.reset();
		notification.reset();

		IndicatorFixture::TearDown();
	}

};


TEST_F(IndicatorTest, PhoneMenu) {
	setMenu("/com/canonical/indicator/sound/phone");

	EXPECT_EVENTUALLY_MENU_ATTRIB(std::vector<int>({0}), "action", "indicator.root");
	EXPECT_MENU_ATTRIB({0}, "x-canonical-type", "com.canonical.indicator.root");
	EXPECT_MENU_ATTRIB({0}, "x-canonical-scroll-action", "indicator.scroll");
	EXPECT_MENU_ATTRIB({0}, "x-canonical-secondary-action", "indicator.mute");

	EXPECT_MENU_ATTRIB(std::vector<int>({0, 0, 0}), "action", "indicator.silent-mode");
	EXPECT_MENU_ATTRIB(std::vector<int>({0, 0, 0}), "label", "Silent Mode");

	EXPECT_MENU_ATTRIB(std::vector<int>({0, 1}), "action", "indicator.phone-settings");
	EXPECT_MENU_ATTRIB(std::vector<int>({0, 1}), "label", "Sound Settings…");
}

TEST_F(IndicatorTest, DesktopMenu) {
	setMenu("/com/canonical/indicator/sound/desktop");

	EXPECT_MENU_ATTRIB({0}, "action", "indicator.root");
	EXPECT_MENU_ATTRIB({0}, "x-canonical-type", "com.canonical.indicator.root");
	EXPECT_MENU_ATTRIB({0}, "x-canonical-scroll-action", "indicator.scroll");
	EXPECT_MENU_ATTRIB({0}, "x-canonical-secondary-action", "indicator.mute");

	EXPECT_MENU_ATTRIB(std::vector<int>({0, 0, 0}), "action", "indicator.mute");
	EXPECT_MENU_ATTRIB(std::vector<int>({0, 0, 0}), "label", "Mute");

	EXPECT_MENU_ATTRIB(std::vector<int>({0, 1}), "action", "indicator.desktop-settings");
	EXPECT_MENU_ATTRIB(std::vector<int>({0, 1}), "label", "Sound Settings…");
}

TEST_F(IndicatorTest, BaseActions) {
	setActions("/com/canonical/indicator/sound");

	ASSERT_ACTION_EXISTS("root");
	ASSERT_ACTION_STATE_TYPE("root", G_VARIANT_TYPE("a{sv}"));

	ASSERT_ACTION_EXISTS("scroll");

	ASSERT_ACTION_EXISTS("silent-mode");
	ASSERT_ACTION_STATE_TYPE("silent-mode", G_VARIANT_TYPE_BOOLEAN);
	EXPECT_ACTION_STATE("silent-mode", false);

	ASSERT_ACTION_EXISTS("mute");
	ASSERT_ACTION_STATE_TYPE("mute", G_VARIANT_TYPE_BOOLEAN);

	ASSERT_ACTION_EXISTS("mic-volume");
	ASSERT_ACTION_STATE_TYPE("mic-volume", G_VARIANT_TYPE_DOUBLE);

	ASSERT_ACTION_EXISTS("volume");
	ASSERT_ACTION_STATE_TYPE("volume", G_VARIANT_TYPE_DOUBLE);
}

