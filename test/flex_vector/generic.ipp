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

#include "../dada.hpp"
#include "../util.hpp"

#include <catch.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/range/irange.hpp>

#include <algorithm>
#include <numeric>
#include <vector>
#include <array>

#ifndef FLEX_VECTOR_T
#error "define the vector template to use in FLEX_VECTOR_T"
#endif

#ifndef VECTOR_T
#error "define the vector template to use in VECTOR_T"
#endif

auto make_test_flex_vector(std::size_t min, std::size_t max)
{
    auto v = FLEX_VECTOR_T<unsigned>{};
    for (auto i = min; i < max; ++i)
        v = v.push_back(i);
    return v;
}

template <typename V=FLEX_VECTOR_T<unsigned>>
auto make_test_flex_vector_front(unsigned min, unsigned max)
{
    auto v = V{};
    for (auto i = max; i > min;)
        v = v.push_front({--i});
    return v;
}

template <std::size_t N>
auto make_many_test_flex_vector()
{
    using vektor_t = FLEX_VECTOR_T<unsigned>;
    auto many = std::array<vektor_t, N>{};
    std::generate_n(
        many.begin(), N,
        [v = vektor_t{}, i = 0u] () mutable
        {
            auto r = v;
            v = v.push_back(i++);
            return r;
        });
    return many;
}

template <std::size_t N>
auto make_many_test_flex_vector_front()
{
    using vektor_t = FLEX_VECTOR_T<unsigned>;
    auto many = std::array<vektor_t, N>{};
    std::generate_n(
        many.begin(), N,
        [i = 0u] () mutable
        {
            return make_test_flex_vector_front(0, i++);
        });
    return many;
}

template <std::size_t N>
auto make_many_test_flex_vector_front_remainder()
{
    using vektor_t = FLEX_VECTOR_T<unsigned>;
    auto many = std::array<vektor_t, N>{};
    std::generate_n(
        many.begin(), N,
        [v = vektor_t{}, i = N-1] () mutable
        {
            auto r = v;
            v = v.push_front(--i);
            return r;
        });
    return many;
}

TEST_CASE("assoc relaxed")
{
    auto v = make_test_flex_vector_front(0, 666u);
    for (auto i = 0u; i < v.size(); ++i) {
        v = v.assoc(i, i+1);
        CHECK(v[i] == i+1);
    }
}

TEST_CASE("push_front")
{
    const auto n = 666u;
    auto v = FLEX_VECTOR_T<unsigned>{};

    for (auto i = 0u; i < n; ++i) {
        v = v.push_front(i);
        CHECK(v.size() == i + 1);
        for (auto j = 0u; j < v.size(); ++j)
            CHECK(v[v.size() - j - 1] == j);
    }
}

TEST_CASE("concat")
{
#if IMMER_SLOW_TESTS
    constexpr auto n = 666u;
#else
    constexpr auto n = 101u;
#endif

    auto all_lhs = make_many_test_flex_vector<n>();
    auto all_rhs = make_many_test_flex_vector_front_remainder<n>();

    SECTION("regular plus regular")
    {
        for (auto i : test_irange(0u, n)) {
            auto c = all_lhs[i] + all_lhs[i];
            CHECK_VECTOR_EQUALS(c, boost::join(
                                    boost::irange(0u, i),
                                    boost::irange(0u, i)));
        }
    }

    SECTION("regular plus relaxed")
    {
        for (auto i : test_irange(0u, n)) {
            auto c = all_lhs[i] + all_rhs[n - i - 1];
            CHECK_VECTOR_EQUALS(c, boost::irange(0u, n - 1));
        }
    }
}

auto make_flex_vector_concat(std::size_t min, std::size_t max)
{
    using vektor_t = FLEX_VECTOR_T<unsigned>;

    if (max == min)
        return vektor_t{};
    else if (max == min + 1)
        return vektor_t{}.push_back(min);
    else {
        auto mid = min + (max - min) / 2;
        return make_flex_vector_concat(min, mid)
            +  make_flex_vector_concat(mid, max);
    }
}

TEST_CASE("concat recursive")
{
    const auto n = 666u;
    auto v = make_flex_vector_concat(0, n);
    CHECK_VECTOR_EQUALS(v, boost::irange(0u, n));
}

TEST_CASE("reduce relaxed")
{
    SECTION("sum")
    {
        const auto n = 666u;
        auto v = make_test_flex_vector_front(0, n);

        auto sum = v.reduce(std::plus<unsigned>{}, 0u);
        auto expected = v.size() * (v.size() - 1) / 2;
        CHECK(sum == expected);
    }

    SECTION("sum complex")
    {
        const auto n = 20u;

        auto v  = FLEX_VECTOR_T<unsigned>{};
        for (auto i = 0u; i < n; ++i)
            v = v.push_front(i) + v;

        auto sum = v.reduce(std::plus<unsigned>{}, 0u);
        auto expected = (1 << n) - n - 1;
        CHECK(sum == expected);
    }
}

TEST_CASE("take relaxed")
{
    const auto n = 666u;
    auto v = make_test_flex_vector_front(0, n);

    for (auto i : test_irange(0u, n)) {
        auto vv = v.take(i);
        CHECK_VECTOR_EQUALS_RANGE(vv, v.begin(), v.begin() + i);
    }
}

TEST_CASE("drop")
{
    const auto n = 666u;

    SECTION("regular")
    {
        auto v = make_test_flex_vector(0, n);

        for (auto i : test_irange(0u, n)) {
            auto vv = v.drop(i);
            CHECK_VECTOR_EQUALS_RANGE(vv, v.begin() + i, v.end());
        }
    }

    SECTION("relaxed")
    {
        auto v = make_test_flex_vector_front(0, n);

        for (auto i : test_irange(0u, n)) {
            auto vv = v.drop(i);
            CHECK_VECTOR_EQUALS_RANGE(vv, v.begin() + i, v.end());
        }
    }
}

#if IMMER_SLOW_TESTS
TEST_CASE("reconcat")
{
    constexpr auto n = 666u;
    auto v = make_test_flex_vector_front(0, n);
    auto all_lhs = make_many_test_flex_vector_front<n + 1>();
    auto all_rhs = make_many_test_flex_vector_front_remainder<n + 1>();

    for (auto i = 0u; i < n; ++i) {
        auto vv = all_lhs[i] + all_rhs[n - i];
        CHECK_VECTOR_EQUALS(vv, v);
    }
}

TEST_CASE("reconcat drop")
{
    constexpr auto n = 666u;
    auto v = make_test_flex_vector_front(0, n);
    auto all_lhs = make_many_test_flex_vector_front<n + 1>();

    for (auto i = 0u; i < n; ++i) {
        auto vv = all_lhs[i] + v.drop(i);
        CHECK_VECTOR_EQUALS(vv, v);
    }
}

TEST_CASE("reconcat take")
{
    constexpr auto n = 666u;
    auto v = make_test_flex_vector_front(0, n);
    auto all_rhs = make_many_test_flex_vector_front_remainder<n + 1>();

    for (auto i = 0u; i < n; ++i) {
        auto vv = v.take(i) + all_rhs[n - i];
        CHECK_VECTOR_EQUALS(vv, v);
    }
}
#endif

TEST_CASE("reconcat take drop")
{
    const auto n = 666u;
    auto v = make_test_flex_vector_front(0, n);

    for (auto i : test_irange(0u, n)) {
        auto vv = v.take(i) + v.drop(i);
        CHECK_VECTOR_EQUALS(vv, v);
    }
}

TEST_CASE("reconcat take drop feedback")
{
    const auto n = 666u;
    auto v = make_test_flex_vector_front(0, n);
    auto vv = v;
    for (auto i : test_irange(0u, n)) {
        vv = vv.take(i) + vv.drop(i);
        CHECK_VECTOR_EQUALS(vv, v);
    }
}

TEST_CASE("iterator relaxed")
{
    const auto n = 666u;
    auto v = make_test_flex_vector_front(0, n);

    SECTION("works with range loop")
    {
        auto i = 0u;
        for (const auto& x : v)
            CHECK(x == i++);
        CHECK(i == v.size());
    }

    SECTION("works with standard algorithms")
    {
        auto s = std::vector<unsigned>(n);
        std::iota(s.begin(), s.end(), 0u);
        std::equal(v.begin(), v.end(), s.begin(), s.end());
    }

    SECTION("can go back from end")
    {
        auto expected  = n - 1;
        CHECK(expected == *--v.end());
    }

    SECTION("works with reversed range adaptor")
    {
        auto r = v | boost::adaptors::reversed;
        auto i = n;
        for (const auto& x : r)
            CHECK(x == --i);
    }

    SECTION("works with strided range adaptor")
    {
        auto r = v | boost::adaptors::strided(5);
        auto i = 0u;
        for (const auto& x : r)
            CHECK(x == 5 * i++);
    }

    SECTION("works reversed")
    {
        auto i = n;
        for (auto iter = v.rbegin(), last = v.rend(); iter != last; ++iter)
            CHECK(*iter == --i);
    }

    SECTION("advance and distance")
    {
        auto i1 = v.begin();
        auto i2 = i1 + 100;
        CHECK(100u == *i2);
        CHECK(100  == i2 - i1);
        CHECK(50u  == *(i2 - 50));
        CHECK(-30  == (i2 - 30) - i2);
    }
}

TEST_CASE("adopt regular vector contents")
{
    const auto n = 666u;
    auto v = VECTOR_T<unsigned>{};
    for (auto i = 0u; i < n; ++i) {
        v = v.push_back(i);
        auto fv = FLEX_VECTOR_T<unsigned>{v};
        CHECK_VECTOR_EQUALS_X(v, fv, [] (auto&& v) { return &v; });
    }
}


TEST_CASE("exception safety relaxed")
{
    using dadaist_vector_t = typename dadaist_vector<FLEX_VECTOR_T<unsigned>>::type;
    constexpr auto n = 666u;

    SECTION("push back")
    {
        auto half = n / 2;
        auto v = make_test_flex_vector_front<dadaist_vector_t>(0, half);
        auto d = dadaism{};
        for (auto i = half; v.size() < n;) {
            d.next();
            try {
                v = v.push_back({i});
                ++i;
            } catch (dada_error) {}
            CHECK_VECTOR_EQUALS(v, boost::irange(0u, i));
        }
        CHECK(d.happenings > 0);
        IMMER_TRACE_E(d.happenings);
    }

    SECTION("update")
    {
        auto v = make_test_flex_vector_front<dadaist_vector_t>(0, n);
        auto d = dadaism{};
        for (auto i = 0u; i < n;) {
            d.next();
            try {
                v = v.update(i, [] (auto x) { return dada(), x + 1; });
                ++i;
            } catch (dada_error) {}
            CHECK_VECTOR_EQUALS(v, boost::join(
                                    boost::irange(1u, 1u + i),
                                    boost::irange(i, n)));
        }
        CHECK(d.happenings > 0);
        IMMER_TRACE_E(d.happenings);
    }
}
