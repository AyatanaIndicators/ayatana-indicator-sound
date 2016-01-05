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

#include <chrono>
#include <map>
#include <sstream>
#include <iostream>

using namespace std;

namespace unity
{

namespace gmenuharness
{

namespace
{


static void printLocation(ostream& ss, const vector<unsigned int>& location, bool first)
{
    for (int i : location)
    {
        ss << " ";
        if (first)
        {
            ss << i;
        }
        else
        {
            ss << " ";
        }
    }
    ss << " ";
}

struct compare_vector
{
    bool operator()(const vector<unsigned int>& a,
               const vector<unsigned int>& b) const
    {
        auto p1 = a.begin();
        auto p2 = b.begin();

        while (p1 != a.end())
        {
            if (p2 == b.end())
            {
                return false;
            }
            if (*p2 > *p1)
            {
                return true;
            }
            if (*p1 > *p2)
            {
                return false;
            }

            ++p1;
            ++p2;
        }

        if (p2 != b.end())
        {
            return true;
        }

        return false;
    }
};
}

struct MatchResult::Priv
{
    bool m_success = true;

    map<vector<unsigned int>, vector<string>, compare_vector> m_failures;

    chrono::time_point<chrono::system_clock> m_timeout = chrono::system_clock::now() + chrono::seconds(10);
};

MatchResult::MatchResult() :
        p(new Priv)
{
}

MatchResult::MatchResult(MatchResult&& other)
{
    *this = move(other);
}

MatchResult::MatchResult(const MatchResult& other) :
        p(new Priv)
{
    *this = other;
}

MatchResult& MatchResult::operator=(const MatchResult& other)
{
    p->m_success = other.p->m_success;
    p->m_failures= other.p->m_failures;
    return *this;
}

MatchResult& MatchResult::operator=(MatchResult&& other)
{
    p = move(other.p);
    return *this;
}

MatchResult MatchResult::createChild() const
{
    MatchResult child;
    child.p->m_timeout = p->m_timeout;
    return child;
}

void MatchResult::failure(const vector<unsigned int>& location, const string& message)
{
    p->m_success = false;
    auto it = p->m_failures.find(location);
    if (it == p->m_failures.end())
    {
        it = p->m_failures.insert(make_pair(location, vector<string>())).first;
    }
    it->second.emplace_back(message);
}

void MatchResult::merge(const MatchResult& other)
{
    p->m_success &= other.p->m_success;
    for (const auto& e : other.p->m_failures)
    {
        p->m_failures.insert(make_pair(e.first, e.second));
    }
}

bool MatchResult::success() const
{
    return p->m_success;
}

bool MatchResult::hasTimedOut() const
{
    auto now = chrono::system_clock::now();
    return (now >= p->m_timeout);
}

string MatchResult::concat_failures() const
{
    stringstream ss;
    ss << "Failed expectations:" << endl;
    for (const auto& failure : p->m_failures)
    {
        bool first = true;
        for (const string& s: failure.second)
        {
            printLocation(ss, failure.first, first);
            first = false;
            ss << s << endl;
        }
    }
    return ss.str();
}

}   // namespace gmenuharness

}   // namespace unity
