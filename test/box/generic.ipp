//
// immer - immutable data structures for C++
// Copyright (C) 2017 Juan Pedro Bolivar Puente
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

#ifndef BOX_T
#error "define the box template to use in BOX_T"
#endif

#include <catch.hpp>

TEST_CASE("construction and copy")
{
    auto x = BOX_T<int>{};
    CHECK(x == 0);

    auto y = x;
    CHECK(&x.get() == &y.get());

    auto z = std::move(x);
    CHECK(&z.get() == &y.get());
}

TEST_CASE("equality")
{
    auto x = BOX_T<int>{};
    auto y = x;
    CHECK(x == 0.0f);
    CHECK(x == y);
    CHECK(x == BOX_T<int>{});
    CHECK(x != BOX_T<int>{42});
}

TEST_CASE("update")
{
    auto x = BOX_T<int>{};
    auto y = x.update([](auto v) { return v + 1; });
    CHECK(x == 0);
    CHECK(y == 1);
}

TEST_CASE("update move")
{
    auto x = BOX_T<int>{};
    auto addr = &x.get();
    auto y = std::move(x).update([](auto&& v) {
                                     return std::forward<decltype(v)>(v) + 1;
                                 });

    CHECK(y == 1);
    if (std::is_empty<typename BOX_T<int>::memory_policy::refcount>::value) {
        CHECK(&y.get() != addr);
    } else {
        CHECK(&y.get() == addr);
    }
}
