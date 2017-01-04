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

#include "../util.hpp"

#include <catch.hpp>

#ifndef VECTOR_T
#error "define the vector template to use in VECTOR_T"
#endif

#ifndef TRANSIENT_VECTOR_T
#error "define the vector template to use in TRANSIENT_VECTOR_T"
#endif

template <typename V=VECTOR_T<unsigned>>
auto make_test_vector(unsigned min, unsigned max)
{
    auto v = V{};
    for (auto i = min; i < max; ++i)
        v = v.push_back({i});
    return v;
}

TEST_CASE("from vector and to vector")
{
    constexpr auto n = 100u;

    auto v = make_test_vector(0, n).transient();
    CHECK_VECTOR_EQUALS(v, boost::irange(0u, n));

    auto p = v.persistent();
    CHECK_VECTOR_EQUALS(p, boost::irange(0u, n));
}


TEST_CASE("push back")
{
    SECTION("many elements")
    {
        const auto n = 666u;
        auto v = TRANSIENT_VECTOR_T<unsigned>{};
        for (auto i = 0u; i < n; ++i) {
            v.push_back(i * 42);
            CHECK(v.size() == i + 1);
            for (auto j = 0u; j < v.size(); ++j)
                CHECK(v[j] == j * 42);
        }
    }
}
