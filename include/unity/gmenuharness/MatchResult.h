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

#include <vector>
#include <memory>
#include <string>

namespace unity
{

namespace gmenuharness
{

class MatchResult
{
public:
    MatchResult();

    MatchResult(MatchResult&& other);

    MatchResult(const MatchResult& other);

    MatchResult& operator=(const MatchResult& other);

    MatchResult& operator=(MatchResult&& other);

    ~MatchResult() = default;

    MatchResult createChild() const;

    void failure(const std::vector<unsigned int>& location, const std::string& message);

    void merge(const MatchResult& other);

    bool success() const;

    bool hasTimedOut() const;

    std::string concat_failures() const;

protected:
    struct Priv;

    std::shared_ptr<Priv> p;
};

}   // namespace gmenuharness

}   // namespace unity
