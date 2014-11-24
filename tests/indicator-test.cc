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

class IndicatorTest : public IndicatorFixture
{
protected:
	IndicatorTest (void) :
		IndicatorFixture(INDICATOR_SOUND_SERVICE_BINARY, "com.canonical.indicator.sound")
	{
	}

	virtual void SetUp() override
	{
		addMock(buildBustleMock("indicator-test-session.bustle", DBUS_TEST_SERVICE_BUS_SESSION));
		addMock(buildBustleMock("indicator-test-system.bustle", DBUS_TEST_SERVICE_BUS_SYSTEM));

		AccountsServiceMock as;
		addMock(as);

		IndicatorFixture::SetUp();
	}

};


TEST_F(IndicatorTest, PhoneMenu) {
	setMenu("/com/canonical/indicator/sound/phone");

	EXPECT_MENU_ATTRIB({0}, "action", "indicator.root");
	EXPECT_MENU_ATTRIB({0}, "x-canonical-type", "com.canonical.indicator.root");

	EXPECT_MENU_ATTRIB(std::vector<int>({0, 0, 0}), "action", "indicator.silent-mode");
}

TEST_F(IndicatorTest, DesktopMenu) {
	setMenu("/com/canonical/indicator/sound/desktop");

	EXPECT_MENU_ATTRIB({0}, "action", "indicator.root");
}

TEST_F(IndicatorTest, SilentActions) {
	setActions("/com/canonical/indicator/sound");

	expectActionExists("scroll");

	expectActionExists("silent-mode");
	expectActionStateIs("silent-mode", false);

	expectActionExists("mute");
	expectActionStateIs("mute", false);
}
