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

#ifndef VECTOR_TRANSIENT_T
#error "define the vector template to use in VECTOR_TRANSIENT_T"
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
        auto v = VECTOR_TRANSIENT_T<unsigned>{};
        for (auto i = 0u; i < n; ++i) {
            v.push_back(i * 42);
            CHECK(v.size() == i + 1);
            for (auto j = 0u; j < v.size(); ++j)
                CHECK(v[j] == j * 42);
        }
    }
}

TEST_CASE("exception safety")
{
    constexpr auto n = 666u;

    using dadaist_vector_t = typename dadaist_vector<VECTOR_T<unsigned>>::type;

    SECTION("push back")
    {
        auto t = as_transient_tester(dadaist_vector_t{});
        auto d = dadaism{};
        for (auto li = 0u, i = 0u; i < n;) {
            auto s = d.next();
            try {
                if (t.transient)
                    t.vt.push_back({i});
                else
                    t.vp = t.vp.push_back({i});
                ++i;
            } catch (dada_error) {}
            if (t.step())
                li = i;
            if (t.transient) {
                CHECK_VECTOR_EQUALS(t.vt, boost::irange(0u, i));
                CHECK_VECTOR_EQUALS(t.vp, boost::irange(0u, li));
            } else {
                CHECK_VECTOR_EQUALS(t.vp, boost::irange(0u, i));
                CHECK_VECTOR_EQUALS(t.vt, boost::irange(0u, li));
            }
        }
        CHECK(d.happenings > 0);
        CHECK(t.d.happenings > 0);
        IMMER_TRACE_E(d.happenings);
        IMMER_TRACE_E(t.d.happenings);
    }

    SECTION("update")
    {
        using boost::join;
        using boost::irange;

        auto t = as_transient_tester(make_test_vector<dadaist_vector_t>(0, n));
        auto d = dadaism{};
        for (auto li = 0u, i = 0u; i < n;) {
            auto s = d.next();
            try {
                if (t.transient)
                    t.vt.update(i, [] (auto x) { return dada(), x + 1; });
                else
                    t.vp = t.vp.update(i, [] (auto x) { return dada(), x + 1; });
                ++i;
            } catch (dada_error) {}
            if (t.step())
                li = i;
            if (t.transient) {
                CHECK_VECTOR_EQUALS(t.vt, join(irange(1u, 1u + i), irange(i, n)));
                CHECK_VECTOR_EQUALS(t.vp, join(irange(1u, 1u + li), irange(li, n)));
            } else {
                CHECK_VECTOR_EQUALS(t.vp, join(irange(1u, 1u + i), irange(i, n)));
                CHECK_VECTOR_EQUALS(t.vt, join(irange(1u, 1u + li), irange(li, n)));
            }
        }
        CHECK(d.happenings > 0);
        CHECK(t.d.happenings > 0);
    }
}
