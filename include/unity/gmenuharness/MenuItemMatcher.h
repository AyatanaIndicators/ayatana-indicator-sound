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

#include <map>
#include <memory>
#include <string>

#include <gio/gio.h>

namespace unity
{

namespace gmenuharness
{

class MatchResult;

class MenuItemMatcher
{
public:
    enum class Mode
    {
        all,
        starts_with,
        ends_with
    };

    enum class Type
    {
        plain,
        checkbox,
        radio
    };

    static MenuItemMatcher checkbox();

    static MenuItemMatcher radio();

    MenuItemMatcher();

    ~MenuItemMatcher();

    MenuItemMatcher(const MenuItemMatcher& other);

    MenuItemMatcher(MenuItemMatcher&& other);

    MenuItemMatcher& operator=(const MenuItemMatcher& other);

    MenuItemMatcher& operator=(MenuItemMatcher&& other);

    MenuItemMatcher& type(Type type);

    MenuItemMatcher& label(const std::string& label);

    MenuItemMatcher& action(const std::string& action);

    MenuItemMatcher& state_icons(const std::vector<std::string>& state);

    MenuItemMatcher& icon(const std::string& icon);

    MenuItemMatcher& themed_icon(const std::string& iconName, const std::vector<std::string>& icons);

    MenuItemMatcher& widget(const std::string& widget);

    MenuItemMatcher& pass_through_attribute(const std::string& actionName, const std::shared_ptr<GVariant>& value);

    MenuItemMatcher& pass_through_boolean_attribute(const std::string& actionName, bool value);

    MenuItemMatcher& pass_through_string_attribute(const std::string& actionName, const std::string& value);

    MenuItemMatcher& pass_through_double_attribute(const std::string& actionName, double value);

    MenuItemMatcher& round_doubles(double maxDifference);

    MenuItemMatcher& attribute(const std::string& name, const std::shared_ptr<GVariant>& value);

    MenuItemMatcher& boolean_attribute(const std::string& name, bool value);

    MenuItemMatcher& string_attribute(const std::string& name, const std::string& value);

    MenuItemMatcher& int32_attribute(const std::string& name, int value);

    MenuItemMatcher& int64_attribute(const std::string& name, int value);

    MenuItemMatcher& double_attribute(const std::string& name, double value);

    MenuItemMatcher& toggled(bool toggled);

    MenuItemMatcher& mode(Mode mode);

    MenuItemMatcher& submenu();

    MenuItemMatcher& section();

    MenuItemMatcher& is_empty();

    MenuItemMatcher& has_exactly(std::size_t children);

    MenuItemMatcher& item(const MenuItemMatcher& item);

    MenuItemMatcher& item(MenuItemMatcher&& item);

    MenuItemMatcher& pass_through_activate(const std::string& action, const std::shared_ptr<GVariant>& parameter = nullptr);

    MenuItemMatcher& activate(const std::shared_ptr<GVariant>& parameter = nullptr);

    MenuItemMatcher& set_pass_through_action_state(const std::string& action, const std::shared_ptr<GVariant>& state);

    MenuItemMatcher& set_action_state(const std::shared_ptr<GVariant>& state);

    void match(MatchResult& matchResult, const std::vector<unsigned int>& location,
          const std::shared_ptr<GMenuModel>& menu,
          std::map<std::string, std::shared_ptr<GActionGroup>>& actions,
          int index) const;

protected:
    struct Priv;

    std::shared_ptr<Priv> p;
};

}   // namespace gmenuharness

}   // namespace unity
