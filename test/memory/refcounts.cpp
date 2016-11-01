//
// immer - immutable data structures for C++
// Copyright (C) 2016 Juan Pedro Bolivar Puente
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

#include <immer/refcount/refcount_policy.hpp>
#include <immer/refcount/unsafe_refcount_policy.hpp>
#include <immer/refcount/no_refcount_policy.hpp>

#include <catch.hpp>

TEST_CASE("no refcount has no data")
{
    static_assert(std::is_empty<immer::no_refcount_policy::data>{}, "");
}

template <typename RefcountPolicy>
void test_refcount()
{
    using refcount = RefcountPolicy;
    using data_t = typename refcount::data;

    SECTION("starts at one")
    {
        data_t elem{};
        CHECK(refcount::dec(&elem));
    }

    SECTION("disowned starts at zero")
    {
        data_t elem{immer::disowned{}};
        refcount::inc(&elem);
        CHECK(refcount::dec(&elem));
    }

    SECTION("inc dec")
    {
        data_t elem{};
        refcount::inc(&elem);
        CHECK(!refcount::dec(&elem));
        CHECK(refcount::dec(&elem));
    }

    SECTION("inc dec unsafe")
    {
        data_t elem{};
        refcount::inc(&elem);
        CHECK(!refcount::dec(&elem));
        refcount::inc(&elem);
        refcount::dec_unsafe(&elem);
        CHECK(refcount::dec(&elem));
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
