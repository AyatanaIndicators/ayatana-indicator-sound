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

#include <unity/gmenuharness/MatchResult.h>
#include <unity/gmenuharness/MatchUtils.h>
#include <unity/gmenuharness/MenuItemMatcher.h>

#include <iostream>
#include <vector>
#include <map>

using namespace std;

namespace unity
{

namespace gmenuharness
{

namespace
{

enum class LinkType
{
    any,
    section,
    submenu
};

static string bool_to_string(bool value)
{
    return value? "true" : "false";
}

static shared_ptr<GVariant> get_action_group_attribute(const shared_ptr<GActionGroup>& actionGroup, const gchar* attribute)
{
    shared_ptr<GVariant> value(
            g_action_group_get_action_state(actionGroup.get(), attribute),
            &gvariant_deleter);
    return value;
}

static shared_ptr<GVariant> get_attribute(const shared_ptr<GMenuItem> menuItem, const gchar* attribute)
{
    shared_ptr<GVariant> value(
            g_menu_item_get_attribute_value(menuItem.get(), attribute, nullptr),
            &gvariant_deleter);
    return value;
}

static string get_string_attribute(const shared_ptr<GMenuItem> menuItem, const gchar* attribute)
{
    string result;
    char* temp = nullptr;
    if (g_menu_item_get_attribute(menuItem.get(), attribute, "s", &temp))
    {
        result = temp;
        g_free(temp);
    }
    return result;
}

static pair<string, string> split_action(const string& action)
{
    auto index = action.find('.');

    if (index == string::npos)
    {
        return make_pair(string(), action);
    }

    return make_pair(action.substr(0, index), action.substr(index + 1, action.size()));
}

static string type_to_string(MenuItemMatcher::Type type)
{
    switch(type)
    {
        case MenuItemMatcher::Type::plain:
            return "plain";
        case MenuItemMatcher::Type::checkbox:
            return "checkbox";
        case MenuItemMatcher::Type::radio:
            return "radio";
    }

    return string();
}
}

struct MenuItemMatcher::Priv
{
    void all(MatchResult& matchResult, const vector<unsigned int>& location,
        const shared_ptr<GMenuModel>& menu,
        map<string, shared_ptr<GActionGroup>>& actions)
    {
        int count = g_menu_model_get_n_items(menu.get());

        if (m_items.size() != (unsigned int) count)
        {
            matchResult.failure(
                    location,
                    "Expected " + to_string(m_items.size())
                            + " children, but found " + to_string(count));
            return;
        }

        for (size_t i = 0; i < m_items.size(); ++i)
        {
            const auto& matcher = m_items.at(i);
            matcher.match(matchResult, location, menu, actions, i);
        }
    }

    void startsWith(MatchResult& matchResult, const vector<unsigned int>& location,
               const shared_ptr<GMenuModel>& menu,
               map<string, shared_ptr<GActionGroup>>& actions)
    {
        int count = g_menu_model_get_n_items(menu.get());
        if (m_items.size() > (unsigned int) count)
        {
            matchResult.failure(
                    location,
                    "Expected at least " + to_string(m_items.size())
                            + " children, but found " + to_string(count));
            return;
        }

        for (size_t i = 0; i < m_items.size(); ++i)
        {
            const auto& matcher = m_items.at(i);
            matcher.match(matchResult, location, menu, actions, i);
        }
    }

    void endsWith(MatchResult& matchResult, const vector<unsigned int>& location,
               const shared_ptr<GMenuModel>& menu,
               map<string, shared_ptr<GActionGroup>>& actions)
    {
        int count = g_menu_model_get_n_items(menu.get());
        if (m_items.size() > (unsigned int) count)
        {
            matchResult.failure(
                    location,
                    "Expected at least " + to_string(m_items.size())
                            + " children, but found " + to_string(count));
            return;
        }

        // match the last N items
        size_t j;
        for (size_t i = count - m_items.size(), j = 0; i < count && j < m_items.size(); ++i, ++j)
        {
            const auto& matcher = m_items.at(j);
            matcher.match(matchResult, location, menu, actions, i);
        }
    }

    Type m_type = Type::plain;

    Mode m_mode = Mode::all;

    LinkType m_linkType = LinkType::any;

    shared_ptr<size_t> m_expectedSize;

    shared_ptr<string> m_label;

    shared_ptr<string> m_icon;

    map<shared_ptr<string>, vector<std::string>> m_themed_icons;

    shared_ptr<string> m_action;

    vector<std::string> m_state_icons;

    vector<pair<string, shared_ptr<GVariant>>> m_attributes;

    vector<string> m_not_exist_attributes;

    vector<pair<string, shared_ptr<GVariant>>> m_pass_through_attributes;

    shared_ptr<bool> m_isToggled;

    vector<MenuItemMatcher> m_items;

    vector<pair<string, shared_ptr<GVariant>>> m_activations;

    vector<pair<string, shared_ptr<GVariant>>> m_setActionStates;

    double m_maxDifference = 0.0;
};

MenuItemMatcher MenuItemMatcher::checkbox()
{
    MenuItemMatcher matcher;
    matcher.type(Type::checkbox);
    return matcher;
}

MenuItemMatcher MenuItemMatcher::radio()
{
    MenuItemMatcher matcher;
    matcher.type(Type::radio);
    return matcher;
}

MenuItemMatcher::MenuItemMatcher() :
        p(new Priv)
{
}

MenuItemMatcher::~MenuItemMatcher()
{
}

MenuItemMatcher::MenuItemMatcher(const MenuItemMatcher& other) :
        p(new Priv)
{
    *this = other;
}

MenuItemMatcher::MenuItemMatcher(MenuItemMatcher&& other)
{
    *this = move(other);
}

MenuItemMatcher& MenuItemMatcher::operator=(const MenuItemMatcher& other)
{
    p->m_type = other.p->m_type;
    p->m_mode = other.p->m_mode;
    p->m_expectedSize = other.p->m_expectedSize;
    p->m_label = other.p->m_label;
    p->m_icon = other.p->m_icon;
    p->m_themed_icons = other.p->m_themed_icons;
    p->m_action = other.p->m_action;
    p->m_state_icons = other.p->m_state_icons;
    p->m_attributes = other.p->m_attributes;
    p->m_not_exist_attributes = other.p->m_not_exist_attributes;
    p->m_pass_through_attributes = other.p->m_pass_through_attributes;
    p->m_isToggled = other.p->m_isToggled;
    p->m_linkType = other.p->m_linkType;
    p->m_items = other.p->m_items;
    p->m_activations = other.p->m_activations;
    p->m_setActionStates = other.p->m_setActionStates;
    p->m_maxDifference = other.p->m_maxDifference;
    return *this;
}

MenuItemMatcher& MenuItemMatcher::operator=(MenuItemMatcher&& other)
{
    p = move(other.p);
    return *this;
}

MenuItemMatcher& MenuItemMatcher::type(Type type)
{
    p->m_type = type;
    return *this;
}

MenuItemMatcher& MenuItemMatcher::label(const string& label)
{
    p->m_label = make_shared<string>(label);
    return *this;
}

MenuItemMatcher& MenuItemMatcher::action(const string& action)
{
    p->m_action = make_shared<string>(action);
    return *this;
}

MenuItemMatcher& MenuItemMatcher::state_icons(const std::vector<std::string>& state_icons)
{
    p->m_state_icons = state_icons;
    return *this;
}

MenuItemMatcher& MenuItemMatcher::icon(const string& icon)
{
    p->m_icon = make_shared<string>(icon);
    return *this;
}

MenuItemMatcher& MenuItemMatcher::themed_icon(const std::string& iconName, const std::vector<std::string>& icons)
{
    p->m_themed_icons[make_shared<string>(iconName)] = icons;
    return *this;
}

MenuItemMatcher& MenuItemMatcher::widget(const string& widget)
{
    return string_attribute("x-canonical-type", widget);
}

MenuItemMatcher& MenuItemMatcher::pass_through_attribute(const string& actionName, const shared_ptr<GVariant>& value)
{
    p->m_pass_through_attributes.emplace_back(actionName, value);
    return *this;
}

MenuItemMatcher& MenuItemMatcher::pass_through_boolean_attribute(const string& actionName, bool value)
{
    return pass_through_attribute(
            actionName,
            shared_ptr<GVariant>(g_variant_new_boolean(value),
                                 &gvariant_deleter));
}

MenuItemMatcher& MenuItemMatcher::pass_through_string_attribute(const string& actionName, const string& value)
{
    return pass_through_attribute(
            actionName,
            shared_ptr<GVariant>(g_variant_new_string(value.c_str()),
                                 &gvariant_deleter));
}

MenuItemMatcher& MenuItemMatcher::pass_through_double_attribute(const std::string& actionName, double value)
{
    return pass_through_attribute(
                actionName,
                shared_ptr<GVariant>(g_variant_new_double(value),
                                     &gvariant_deleter));
}

MenuItemMatcher& MenuItemMatcher::round_doubles(double maxDifference)
{
    p->m_maxDifference = maxDifference;
    return *this;
}

MenuItemMatcher& MenuItemMatcher::attribute(const string& name, const shared_ptr<GVariant>& value)
{
    p->m_attributes.emplace_back(name, value);
    return *this;
}

MenuItemMatcher& MenuItemMatcher::boolean_attribute(const string& name, bool value)
{
    return attribute(
            name,
            shared_ptr<GVariant>(g_variant_new_boolean(value),
                                 &gvariant_deleter));
}

MenuItemMatcher& MenuItemMatcher::string_attribute(const string& name, const string& value)
{
    return attribute(
            name,
            shared_ptr<GVariant>(g_variant_new_string(value.c_str()),
                                 &gvariant_deleter));
}

MenuItemMatcher& MenuItemMatcher::int32_attribute(const std::string& name, int value)
{
    return attribute(
            name,
            shared_ptr<GVariant>(g_variant_new_int32 (value),
                                 &gvariant_deleter));
}

MenuItemMatcher& MenuItemMatcher::int64_attribute(const std::string& name, int value)
{
    return attribute(
            name,
            shared_ptr<GVariant>(g_variant_new_int64 (value),
                                 &gvariant_deleter));
}

MenuItemMatcher& MenuItemMatcher::double_attribute(const std::string& name, double value)
{
    return attribute(
            name,
            shared_ptr<GVariant>(g_variant_new_double (value),
                                 &gvariant_deleter));
}

MenuItemMatcher& MenuItemMatcher::attribute_not_set(const std::string& name)
{
    p->m_not_exist_attributes.emplace_back (name);
    return *this;
}

MenuItemMatcher& MenuItemMatcher::toggled(bool isToggled)
{
    p->m_isToggled = make_shared<bool>(isToggled);
    return *this;
}

MenuItemMatcher& MenuItemMatcher::mode(Mode mode)
{
    p->m_mode = mode;
    return *this;
}

MenuItemMatcher& MenuItemMatcher::submenu()
{
    p->m_linkType = LinkType::submenu;
    return *this;
}

MenuItemMatcher& MenuItemMatcher::section()
{
    p->m_linkType = LinkType::section;
    return *this;
}

MenuItemMatcher& MenuItemMatcher::is_empty()
{
    return has_exactly(0);
}

MenuItemMatcher& MenuItemMatcher::has_exactly(size_t children)
{
    p->m_expectedSize = make_shared<size_t>(children);
    return *this;
}

MenuItemMatcher& MenuItemMatcher::item(const MenuItemMatcher& item)
{
    p->m_items.emplace_back(item);
    return *this;
}

MenuItemMatcher& MenuItemMatcher::item(MenuItemMatcher&& item)
{
    p->m_items.emplace_back(item);
    return *this;
}

MenuItemMatcher& MenuItemMatcher::pass_through_activate(std::string const& action, const shared_ptr<GVariant>& parameter)
{
    p->m_activations.emplace_back(action, parameter);
    return *this;
}

MenuItemMatcher& MenuItemMatcher::activate(const shared_ptr<GVariant>& parameter)
{
    p->m_activations.emplace_back(string(), parameter);
    return *this;
}

MenuItemMatcher& MenuItemMatcher::set_pass_through_action_state(const std::string& action, const std::shared_ptr<GVariant>& state)
{
    p->m_setActionStates.emplace_back(action, state);
    return *this;
}

MenuItemMatcher& MenuItemMatcher::set_action_state(const std::shared_ptr<GVariant>& state)
{
    p->m_setActionStates.emplace_back("", state);
    return *this;
}

void MenuItemMatcher::match(
        MatchResult& matchResult,
       const vector<unsigned int>& parentLocation,
       const shared_ptr<GMenuModel>& menu,
       map<string, shared_ptr<GActionGroup>>& actions,
       int index) const
{
    shared_ptr<GMenuItem> menuItem(g_menu_item_new_from_model(menu.get(), index), &g_object_deleter);

    vector<unsigned int> location(parentLocation);
    location.emplace_back(index);

    string action = get_string_attribute(menuItem, G_MENU_ATTRIBUTE_ACTION);

    bool isCheckbox = false;
    bool isRadio = false;
    bool isToggled = false;

    pair<string, string> idPair;
    shared_ptr<GActionGroup> actionGroup;
    shared_ptr<GVariant> state;

    if (!action.empty())
    {
        idPair = split_action(action);
        actionGroup = actions[idPair.first];
        state = shared_ptr<GVariant>(g_action_group_get_action_state(actionGroup.get(),
                                     idPair.second.c_str()),
                                     &gvariant_deleter);
        auto attributeTarget = get_attribute(menuItem, G_MENU_ATTRIBUTE_TARGET);

        if (attributeTarget && state)
        {
            isToggled = g_variant_equal(state.get(), attributeTarget.get());
            isRadio = true;
        }
        else if (state
                && g_variant_is_of_type(state.get(), G_VARIANT_TYPE_BOOLEAN))
        {
            isToggled = g_variant_get_boolean(state.get());
            isCheckbox = true;
        }
    }

    Type actualType = Type::plain;
    if (isCheckbox)
    {
        actualType = Type::checkbox;
    }
    else if (isRadio)
    {
        actualType = Type::radio;
    }

    if (actualType != p->m_type)
    {
        matchResult.failure(
                location,
                "Expected " + type_to_string(p->m_type) + ", found "
                        + type_to_string(actualType));
    }

    // check themed icons
    map<shared_ptr<string>, vector<string>>::iterator iter;
    for (iter = p->m_themed_icons.begin(); iter != p->m_themed_icons.end(); ++iter)
    {
        auto icon_val = g_menu_item_get_attribute_value(menuItem.get(), (*iter).first->c_str(), nullptr);
        if (!icon_val)
        {
            matchResult.failure(
                            location,
                            "Expected themed icon " + (*(*iter).first) + " was not found");
        }

        auto gicon = g_icon_deserialize(icon_val);
        if (!gicon || !G_IS_THEMED_ICON(gicon))
        {
            matchResult.failure(
                           location,
                           "Expected attribute " + (*(*iter).first) + " is not a themed icon");
        }
        auto iconNames = g_themed_icon_get_names(G_THEMED_ICON(gicon));
        int nb_icons = 0;
        while(iconNames[nb_icons])
        {
            ++nb_icons;
        }

        if (nb_icons != (*iter).second.size())
        {
            matchResult.failure(
                       location,
                       "Expected " + to_string((*iter).second.size()) +
                       " icons for themed icon [" + (*(*iter).first) +
                       "], but " + to_string(nb_icons) + " were found.");
        }
        else
        {
            // now compare all the icons
            for (int i = 0; i < nb_icons; ++i)
            {
                if (string(iconNames[i]) != (*iter).second[i])
                {
                    matchResult.failure(
                               location,
                               "Icon at position " + to_string(i) +
                               " for themed icon [" + (*(*iter).first) +
                               "], mismatchs. Expected: " + iconNames[i] + " but found " + (*iter).second[i]);
                }
            }
        }
        g_object_unref(gicon);
    }

    string label = get_string_attribute(menuItem, G_MENU_ATTRIBUTE_LABEL);
    if (p->m_label && (*p->m_label) != label)
    {
        matchResult.failure(
                location,
                "Expected label '" + *p->m_label + "', but found '" + label
                        + "'");
    }

    string icon = get_string_attribute(menuItem, G_MENU_ATTRIBUTE_ICON);
    if (p->m_icon && (*p->m_icon) != icon)
    {
        matchResult.failure(
                location,
                "Expected icon '" + *p->m_icon + "', but found '" + icon + "'");
    }

    if (p->m_action && (*p->m_action) != action)
    {
        matchResult.failure(
                location,
                "Expected action '" + *p->m_action + "', but found '" + action
                        + "'");
    }

    if (!p->m_state_icons.empty() && !state)
    {
        matchResult.failure(
                location,
                "Expected state icons but no state was found");
    }
    else if (!p->m_state_icons.empty() && state &&
             !g_variant_is_of_type(state.get(), G_VARIANT_TYPE_VARDICT))
    {
        matchResult.failure(
                location,
                "Expected state icons vardict, found "
                        + type_to_string(actualType));
    }
    else if (!p->m_state_icons.empty() && state &&
             g_variant_is_of_type(state.get(), G_VARIANT_TYPE_VARDICT))
    {
        std::vector<std::string> actual_state_icons;
        GVariantIter it;
        gchar* key;
        GVariant* value;

        g_variant_iter_init(&it, state.get());
        while (g_variant_iter_loop(&it, "{sv}", &key, &value))
        {
            if (std::string(key) == "icon") {
                auto gicon = g_icon_deserialize(value);
                if (gicon && G_IS_THEMED_ICON(gicon))
                {
                    auto iconNames = g_themed_icon_get_names(G_THEMED_ICON(gicon));
                    // Just take the first icon in the list (there is only ever one)
                    actual_state_icons.push_back(iconNames[0]);
                    g_object_unref(gicon);
                }
            }
            else if (std::string(key) == "icons" && g_variant_is_of_type(value, G_VARIANT_TYPE("av")))
            {
                // If we find "icons" in the map, clear any icons we may have found in "icon",
                // then break from the loop as we have found all icons now.
                actual_state_icons.clear();
                GVariantIter icon_it;
                GVariant* icon_value;

                g_variant_iter_init(&icon_it, value);
                while (g_variant_iter_loop(&icon_it, "v", &icon_value))
                {
                    auto gicon = g_icon_deserialize(icon_value);
                    if (gicon && G_IS_THEMED_ICON(gicon))
                    {
                        auto iconNames = g_themed_icon_get_names(G_THEMED_ICON(gicon));
                        // Just take the first icon in the list (there is only ever one)
                        actual_state_icons.push_back(iconNames[0]);
                        g_object_unref(gicon);
                    }
                }
                // We're breaking out of g_variant_iter_loop here so clean up
                g_variant_unref(value);
                g_free(key);
                break;
            }
        }

        if (p->m_state_icons != actual_state_icons)
        {
            std::string expected_icons;
            for (unsigned int i = 0; i < p->m_state_icons.size(); ++i)
            {
                expected_icons += i == 0 ? p->m_state_icons[i] : ", " + p->m_state_icons[i];
            }
            std::string actual_icons;
            for (unsigned int i = 0; i < actual_state_icons.size(); ++i)
            {
                actual_icons += i == 0 ? actual_state_icons[i] : ", " + actual_state_icons[i];
            }
            matchResult.failure(
                    location,
                    "Expected state_icons == {" + expected_icons
                        + "} but found {" + actual_icons + "}");
        }
    }

    for (const auto& e: p->m_pass_through_attributes)
    {
        string actionName = get_string_attribute(menuItem, e.first.c_str());
        if (actionName.empty())
        {
            matchResult.failure(
                    location,
                    "Could not find action name '" + e.first + "'");
        }
        else
        {
            auto passThroughIdPair = split_action(actionName);
            auto actionGroup = actions[passThroughIdPair.first];
            if (actionGroup)
            {
                auto value = get_action_group_attribute(
                        actionGroup, passThroughIdPair.second.c_str());
                if (!value)
                {
                    matchResult.failure(
                            location,
                            "Expected pass-through attribute '" + e.first
                                    + "' was not present");
                }
                else if (!g_variant_is_of_type(e.second.get(), g_variant_get_type(value.get())))
                {
                    std::string expectedType = g_variant_get_type_string(e.second.get());
                    std::string actualType = g_variant_get_type_string(value.get());
                    matchResult.failure(
                            location,
                            "Expected pass-through attribute type '" + expectedType
                                    + "' but found '" + actualType + "'");
                }
                else if (g_variant_compare(e.second.get(), value.get()))
                {
                    bool reportMismatch = true;
                    if (g_strcmp0(g_variant_get_type_string(value.get()),"d") == 0 && p->m_maxDifference)
                    {
                        auto actualDouble = g_variant_get_double(value.get());
                        auto expectedDouble = g_variant_get_double(e.second.get());
                        auto difference = actualDouble-expectedDouble;
                        if (difference < 0) difference = difference * -1.0;
                        if (difference <= p->m_maxDifference)
                        {
                            reportMismatch = false;
                        }
                    }
                    if (reportMismatch)
                    {
                        gchar* expectedString = g_variant_print(e.second.get(), true);
                        gchar* actualString = g_variant_print(value.get(), true);
                        matchResult.failure(
                                location,
                                "Expected pass-through attribute '" + e.first
                                    + "' == " + expectedString + " but found "
                                    + actualString);

                        g_free(expectedString);
                        g_free(actualString);
                    }
                }
            }
            else
            {
                matchResult.failure(location, "Could not find action group for ID '" + passThroughIdPair.first + "'");
            }
        }
    }

    for (const auto& e: p->m_attributes)
    {
        auto value = get_attribute(menuItem, e.first.c_str());
        if (!value)
        {
            matchResult.failure(location,
                    "Expected attribute '" + e.first
                            + "' could not be found");
        }
        else if (!g_variant_is_of_type(e.second.get(), g_variant_get_type(value.get())))
        {
            std::string expectedType = g_variant_get_type_string(e.second.get());
            std::string actualType = g_variant_get_type_string(value.get());
            matchResult.failure(
                    location,
                    "Expected attribute type '" + expectedType
                            + "' but found '" + actualType + "'");
        }
        else if (g_variant_compare(e.second.get(), value.get()))
        {
            gchar* expectedString = g_variant_print(e.second.get(), true);
            gchar* actualString = g_variant_print(value.get(), true);
            matchResult.failure(
                    location,
                    "Expected attribute '" + e.first + "' == " + expectedString
                            + ", but found " + actualString);
            g_free(expectedString);
            g_free(actualString);
        }
    }

    for (const auto& e: p->m_not_exist_attributes)
    {
        auto value = get_attribute(menuItem, e.c_str());
        if (value)
        {
            matchResult.failure(location,
                    "Not expected attribute '" + e
                            + "' was found");
        }
    }

    if (p->m_isToggled && (*p->m_isToggled) != isToggled)
    {
        matchResult.failure(
                location,
                "Expected toggled = " + bool_to_string(*p->m_isToggled)
                        + ", but found " + bool_to_string(isToggled));
    }

    if (!matchResult.success())
    {
        return;
    }

    if (!p->m_items.empty() || p->m_expectedSize)
    {
        shared_ptr<GMenuModel> link;

        switch (p->m_linkType)
        {
            case LinkType::any:
            {
                link.reset(g_menu_model_get_item_link(menu.get(), (int) index, G_MENU_LINK_SUBMENU), &g_object_deleter);
                if (!link)
                {
                    link.reset(g_menu_model_get_item_link(menu.get(), (int) index, G_MENU_LINK_SECTION), &g_object_deleter);
                }
                break;
            }
            case LinkType::submenu:
            {
                link.reset(g_menu_model_get_item_link(menu.get(), (int) index, G_MENU_LINK_SUBMENU), &g_object_deleter);
                break;
            }
            case LinkType::section:
            {
                link.reset(g_menu_model_get_item_link(menu.get(), (int) index, G_MENU_LINK_SECTION), &g_object_deleter);
                break;
            }
        }


        if (!link)
        {
            if (p->m_expectedSize)
            {
                matchResult.failure(
                        location,
                        "Expected " + to_string(*p->m_expectedSize)
                                + " children, but found none");
            }
            else
            {
                matchResult.failure(
                        location,
                        "Expected " + to_string(p->m_items.size())
                                + " children, but found none");
            }
            return;
        }
        else
        {
            while (true)
            {
                MatchResult childMatchResult(matchResult.createChild());

                if (p->m_expectedSize
                        && *p->m_expectedSize
                                != (unsigned int) g_menu_model_get_n_items(
                                        link.get()))
                {
                    childMatchResult.failure(
                            location,
                            "Expected " + to_string(*p->m_expectedSize)
                                    + " child items, but found "
                                    + to_string(
                                            g_menu_model_get_n_items(
                                                    link.get())));
                }
                else if (!p->m_items.empty())
                {
                    switch (p->m_mode)
                    {
                        case Mode::all:
                            p->all(childMatchResult, location, link, actions);
                            break;
                        case Mode::starts_with:
                            p->startsWith(childMatchResult, location, link, actions);
                            break;
                        case Mode::ends_with:
                            p->endsWith(childMatchResult, location, link, actions);
                            break;
                    }
                }

                if (childMatchResult.success())
                {
                    matchResult.merge(childMatchResult);
                    break;
                }
                else
                {
                    if (matchResult.hasTimedOut())
                    {
                        matchResult.merge(childMatchResult);
                        break;
                    }
                    menuWaitForItems(link);
                }
            }
        }
    }


    for (const auto& a: p->m_setActionStates)
    {
        auto stateAction = action;
        auto stateIdPair = idPair;
        auto stateActionGroup = actionGroup;
        if (!a.first.empty())
        {
            stateAction = get_string_attribute(menuItem, a.first.c_str());;
            stateIdPair = split_action(stateAction);
            stateActionGroup = actions[stateIdPair.first];
        }

        if (stateAction.empty())
        {
            matchResult.failure(
                    location,
                    "Tried to set action state, but no action was found");
        }
        else if(!stateActionGroup)
        {
            matchResult.failure(
                    location,
                    "Tried to set action state for action group '" + stateIdPair.first
                            + "', but action group wasn't found");
        }
        else if (!g_action_group_has_action(stateActionGroup.get(), stateIdPair.second.c_str()))
        {
            matchResult.failure(
                    location,
                    "Tried to set action state for action '" + stateAction
                            + "', but action was not found");
        }
        else
        {
            g_action_group_change_action_state(stateActionGroup.get(), stateIdPair.second.c_str(),
                                               g_variant_ref(a.second.get()));
        }

        // FIXME this is a dodgy way to ensure the action state change gets dispatched
        menuWaitForItems(menu, 100);
    }

    for (const auto& a: p->m_activations)
    {
        string tmpAction = action;
        auto tmpIdPair = idPair;
        auto tmpActionGroup = actionGroup;
        if (!a.first.empty())
        {
            tmpAction = get_string_attribute(menuItem, a.first.c_str());
            tmpIdPair = split_action(tmpAction);
            tmpActionGroup = actions[tmpIdPair.first];
        }

        if (tmpAction.empty())
        {
            matchResult.failure(
                    location,
                    "Tried to activate action, but no action was found");
        }
        else if(!tmpActionGroup)
        {
            matchResult.failure(
                    location,
                    "Tried to activate action group '" + tmpIdPair.first
                            + "', but action group wasn't found");
        }
        else if (!g_action_group_has_action(tmpActionGroup.get(), tmpIdPair.second.c_str()))
        {
            matchResult.failure(
                    location,
                    "Tried to activate action '" + tmpAction + "', but action was not found");
        }
        else
        {
            if (a.second)
            {
                g_action_group_activate_action(tmpActionGroup.get(), tmpIdPair.second.c_str(),
                                               g_variant_ref(a.second.get()));
            }
            else
            {
                g_action_group_activate_action(tmpActionGroup.get(), tmpIdPair.second.c_str(), nullptr);
            }

            // FIXME this is a dodgy way to ensure the activation gets dispatched
            menuWaitForItems(menu, 100);
        }
    }
}

}   // namepsace gmenuharness

}   // namespace unity
