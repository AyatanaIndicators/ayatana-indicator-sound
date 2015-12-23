/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Pete Woods <pete.woods@canonical.com>
 */

#pragma once

#define EXPECT_MATCHRESULT(statement) \
do {\
    auto result = (statement);\
    GTEST_TEST_BOOLEAN_(result.success(), #statement, false, true, \
                      GTEST_NONFATAL_FAILURE_) << result.concat_failures().c_str(); \
} while (0)

#include <unity/gmenuharness/MatchResult.h>
#include <unity/gmenuharness/MenuItemMatcher.h>

#include <memory>
#include <vector>

namespace unity
{

namespace gmenuharness
{

class MenuMatcher
{
public:
    class Parameters
    {
    public:
        Parameters(
                const std::string& busName,
                const std::vector<std::pair<std::string, std::string>>& actions,
                const std::string& menuObjectPath);

        ~Parameters();

        Parameters(const Parameters& other);

        Parameters(Parameters&& other);

        Parameters& operator=(const Parameters& other);

        Parameters& operator=(Parameters&& other);

    protected:
        friend MenuMatcher;

        struct Priv;

        std::shared_ptr<Priv> p;
    };

    MenuMatcher(const Parameters& parameters);

    ~MenuMatcher();

    MenuMatcher(const MenuMatcher& other) = delete;

    MenuMatcher(MenuMatcher&& other) = delete;

    MenuMatcher& operator=(const MenuMatcher& other) = delete;

    MenuMatcher& operator=(MenuMatcher&& other) = delete;

    MenuMatcher& item(const MenuItemMatcher& item);

    MatchResult match() const;

    void match(MatchResult& matchResult) const;

protected:
    struct Priv;

    std::shared_ptr<Priv> p;
};

}   // gmenuharness

}   // unity
