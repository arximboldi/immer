//
// immer - immutable data structures for C++
// Copyright (C) 2016, 2017 Juan Pedro Bolivar Puente
//
// This file is part of immer.
//
// immer is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// immer is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with immer.  If not, see <http://www.gnu.org/licenses/>.
//

#include <catch.hpp>

#ifndef MAP_T
#error "define the map template to use in MAP_T"
#endif

TEST_CASE("instantiation")
{
    SECTION("default")
    {
        auto v = MAP_T<int, int>{};
        CHECK(v.size() == 0u);
    }
}


TEST_CASE("basic insertion")
{
    auto v1 = MAP_T<int, int>{};
    CHECK(v1.count(42) == 0);

    auto v2 = v1.insert({42, {}});
    CHECK(v1.count(42) == 0);
    CHECK(v2.count(42) == 1);

    auto v3 = v2.insert({42, {}});
    CHECK(v1.count(42) == 0);
    CHECK(v2.count(42) == 1);
    CHECK(v3.count(42) == 1);
};
