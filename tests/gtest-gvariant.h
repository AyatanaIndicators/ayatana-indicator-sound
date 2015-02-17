/*
 * Copyright © 2015 Canonical Ltd.
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

namespace GTestGVariant {

testing::AssertionResult expectVariantEqual (const gchar * expectStr, const gchar * haveStr, GVariant * expect, GVariant * have)
{
	if (expect == nullptr && have == nullptr) {
		auto result = testing::AssertionSuccess();
		return result;
	}

	if (expect == nullptr || have == nullptr) {
		gchar * havePrint;
		if (have == nullptr) {
			havePrint = g_strdup("(nullptr)");
		} else {
			havePrint = g_variant_print(have, TRUE);
		}

		auto result = testing::AssertionFailure();
		result <<
			"    Result: " << haveStr << std::endl <<
			"     Value: " << havePrint << std::endl <<
			"  Expected: " << expectStr << std::endl;

		g_free(havePrint);
		return result;
	}

	if (g_variant_equal(expect, have)) {
		auto result = testing::AssertionSuccess();
		return result;
	} else {
		gchar * havePrint = g_variant_print(have, TRUE);
		gchar * expectPrint = g_variant_print(expect, TRUE);

		auto result = testing::AssertionFailure();
		result <<
			"    Result: " << haveStr << std::endl <<
			"     Value: " << havePrint << std::endl <<
			"  Expected: " << expectStr << std::endl <<
			"  Expected: " << expectPrint << std::endl;

		g_free(havePrint);
		g_free(expectPrint);

		return result;
	}
}

testing::AssertionResult expectVariantEqual (const gchar * expectStr, const gchar * haveStr, std::shared_ptr<GVariant> expect, std::shared_ptr<GVariant> have)
{
	return expectVariantEqual(expectStr, haveStr, expect.get(), have.get());
}

testing::AssertionResult expectVariantEqual (const gchar * expectStr, const gchar * haveStr, const char * expect, std::shared_ptr<GVariant> have)
{
	auto expectv = std::shared_ptr<GVariant>([expect] {
			auto variant = g_variant_parse(nullptr, expect, nullptr, nullptr, nullptr);
			if (variant != nullptr)
				g_variant_ref_sink(variant);
			return variant;
		}(),
		[](GVariant * variant) {
			if (variant != nullptr)
				g_variant_unref(variant);
		});

	return expectVariantEqual(expectStr, haveStr, expectv, have);
}

testing::AssertionResult expectVariantEqual (const gchar * expectStr, const gchar * haveStr, const char * expect, GVariant * have)
{
	auto havep = std::shared_ptr<GVariant>([have] {
			if (have != nullptr)
				g_variant_ref_sink(have);
			return have;
		}(),
		[](GVariant * variant) {
			if (variant != nullptr)
				g_variant_unref(variant);
		});

	return expectVariantEqual(expectStr, haveStr, expect, havep);
}

}; // ns GTestGVariant 

#define EXPECT_GVARIANT_EQ(expect, have) \
	EXPECT_PRED_FORMAT2(GTestGVariant::expectVariantEqual, expect, have)
