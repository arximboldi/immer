//
// immer - immutable data structures for C++
// Copyright (C) 2016, 2017 Juan Pedro Bolivar Puente
//
// This file is part of immer.
//
// immer is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// immer is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with immer.  If not, see <http://www.gnu.org/licenses/>.
//

#include <immer/refcount/refcount_policy.hpp>
#include <immer/refcount/unsafe_refcount_policy.hpp>
#include <immer/refcount/no_refcount_policy.hpp>

#include <catch.hpp>

TEST_CASE("no refcount has no data")
{
    static_assert(std::is_empty<immer::no_refcount_policy>{}, "");
}

template <typename RefcountPolicy>
void test_refcount()
{
    using refcount = RefcountPolicy;

    SECTION("starts at one")
    {
        refcount elem{};
        CHECK(elem.dec());
    }

    SECTION("disowned starts at zero")
    {
        refcount elem{immer::disowned{}};
        elem.inc();
        CHECK(elem.dec());
    }

    SECTION("inc dec")
    {
        refcount elem{};
        elem.inc();
        CHECK(!elem.dec());
        CHECK(elem.dec());
    }

    SECTION("inc dec unsafe")
    {
        refcount elem{};
        elem.inc();
        CHECK(!elem.dec());
        elem.inc();
        elem.dec_unsafe();
        CHECK(elem.dec());
    }
}

TEST_CASE("basic refcount")
{
    test_refcount<immer::refcount_policy>();
}

TEST_CASE("thread unsafe refcount")
{
    test_refcount<immer::unsafe_refcount_policy>();
}
