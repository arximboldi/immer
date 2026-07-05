//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#ifndef SORTED_SET_T
#error "define the set template to use in SORTED_SET_T"
#include <immer/sorted_set.hpp>
#define SORTED_SET_T ::immer::sorted_set
#endif

#include <immer/sorted_set_transient.hpp>

#include "test/util.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <functional>
#include <random>
#include <set>
#include <string>
#include <vector>

IMMER_RANGES_CHECK(std::ranges::bidirectional_range<SORTED_SET_T<std::string>>);

namespace {

template <typename Set, typename Oracle>
void check_equals_oracle(const Set& s, const Oracle& oracle)
{
    REQUIRE(s.impl().check_tree());
    REQUIRE(s.size() == oracle.size());
    REQUIRE(std::equal(s.begin(), s.end(), oracle.begin(), oracle.end()));
}

} // namespace

TEST_CASE("sorted_set: instantiation and basic queries")
{
    auto v = SORTED_SET_T<int>{};
    CHECK(v.size() == 0u);
    CHECK(v.empty());
    CHECK(v.begin() == v.end());
    CHECK(v.count(42) == 0u);
    CHECK(v.find(42) == nullptr);
}

TEST_CASE("sorted_set: constructors")
{
    auto v = SORTED_SET_T<int>{5, 3, 8, 3, 1};
    CHECK(v.size() == 4u);
    CHECK(v.count(3) == 1u);

    auto values = std::vector<int>{4, 2, 9};
    auto u      = SORTED_SET_T<int>{values.begin(), values.end()};
    CHECK(u.size() == 3u);
    CHECK(u.count(9) == 1u);
}

TEST_CASE("sorted_set: insert, erase and find")
{
    auto v = SORTED_SET_T<int>{};
    for (auto i = 0; i < 200; ++i)
        v = v.insert(i * 2);
    CHECK(v.size() == 200u);
    CHECK(v.count(100) == 1u);
    CHECK(v.count(101) == 0u);
    REQUIRE(v.find(100) != nullptr);
    CHECK(*v.find(100) == 100);

    auto u = v.erase(100);
    CHECK(u.size() == 199u);
    CHECK(u.find(100) == nullptr);
    CHECK(v.find(100) != nullptr);
    CHECK(v.erase(101).identity() == v.identity());

    CHECK(v.insert(100).size() == v.size()); // replaces the equivalent value
}

TEST_CASE("sorted_set: iteration is sorted")
{
    auto keys = std::vector<int>{};
    for (auto i = 0; i < 500; ++i)
        keys.push_back(i * 3);
    auto shuffled = keys;
    std::shuffle(
        shuffled.begin(), shuffled.end(), std::default_random_engine{42});

    auto v    = SORTED_SET_T<int>{shuffled.begin(), shuffled.end()};
    auto seen = std::vector<int>{v.begin(), v.end()};
    CHECK(seen == keys);

    auto rseen = std::vector<int>{v.rbegin(), v.rend()};
    std::reverse(rseen.begin(), rseen.end());
    CHECK(rseen == keys);

    CHECK(v.front() == 0);
    CHECK(v.back() == 499 * 3);
}

TEST_CASE("sorted_set: lower_bound and upper_bound")
{
    auto v = SORTED_SET_T<int>{};
    for (auto i = 0; i < 100; ++i)
        v = v.insert(i * 10);

    CHECK(*v.lower_bound(50) == 50);
    CHECK(*v.lower_bound(51) == 60);
    CHECK(*v.upper_bound(50) == 60);
    CHECK(*v.lower_bound(-1) == 0);
    CHECK(v.upper_bound(990) == v.end());

    auto count = 0;
    for (auto it = v.lower_bound(100), e = v.upper_bound(200); it != e; ++it)
        ++count;
    CHECK(count == 11);
}

TEST_CASE("sorted_set: equality")
{
    auto a = SORTED_SET_T<int>{1, 2, 3};
    auto b = SORTED_SET_T<int>{}.insert(3).insert(1).insert(2);
    CHECK(a == b);
    CHECK(!(a != b));
    CHECK(a != b.insert(4));
    CHECK(a != b.erase(2));
    CHECK(a == a.erase(42));
}

TEST_CASE("sorted_set: random operations against std::set")
{
    auto engine  = std::default_random_engine{2026};
    auto key_gen = std::uniform_int_distribution<int>{0, 254};
    auto op_gen  = std::uniform_int_distribution<int>{0, 99};

    auto v      = SORTED_SET_T<int>{};
    auto oracle = std::set<int>{};
    for (auto op = 0; op < 2000; ++op) {
        auto k = key_gen(engine);
        if (op_gen(engine) < 60) {
            v = v.insert(k);
            oracle.insert(k);
        } else {
            v = v.erase(k);
            oracle.erase(k);
        }
        REQUIRE(v.size() == oracle.size());
        if (op % 100 == 0)
            check_equals_oracle(v, oracle);
    }
    check_equals_oracle(v, oracle);
}

TEST_CASE("sorted_set: transparent comparator lookups")
{
    auto v = SORTED_SET_T<std::string, std::less<>>{};
    for (auto i = 0; i < 100; ++i)
        v = v.insert("key" + std::to_string(i));

    CHECK(v.count("key42") == 1u);
    CHECK(v.count("nope") == 0u);
    REQUIRE(v.find("key42") != nullptr);
    CHECK(*v.find("key42") == "key42");
    CHECK(*v.lower_bound("key42") == "key42");
}

TEST_CASE("sorted_set: transient round trip")
{
    auto v = SORTED_SET_T<int>{};
    auto t = v.transient();
    for (auto i = 0; i < 500; ++i)
        t.insert(i);
    CHECK(t.size() == 500u);
    CHECK(t.count(300) == 1u);
    REQUIRE(t.find(499) != nullptr);
    CHECK(*t.lower_bound(100) == 100);

    auto frozen = t.persistent();
    for (auto i = 500; i < 600; ++i)
        t.insert(i);
    t.erase(0);
    auto v2 = std::move(t).persistent();

    REQUIRE(frozen.impl().check_tree());
    REQUIRE(v2.impl().check_tree());
    CHECK(frozen.size() == 500u);
    CHECK(v2.size() == 599u);
    CHECK(frozen.count(599) == 0u);
    CHECK(v2.count(599) == 1u);
    CHECK(frozen.count(0) == 1u);
    CHECK(v2.count(0) == 0u);
    for (auto i = 0; i < 500; ++i)
        REQUIRE(frozen.count(i) == 1u);
}

TEST_CASE("sorted_set: move optimized operations")
{
    auto v = SORTED_SET_T<int>{};
    for (auto i = 0; i < 100; ++i)
        v = std::move(v).insert(i);
    CHECK(v.size() == 100u);
    REQUIRE(v.impl().check_tree());

    v = std::move(v).erase(50);
    CHECK(v.size() == 99u);
    CHECK(v.count(50) == 0u);
    REQUIRE(v.impl().check_tree());
}

TEST_CASE("sorted_set: bigger set")
{
    auto values = std::vector<int>{};
    for (auto i = 0; i < 10000; ++i)
        values.push_back(i);
    std::shuffle(values.begin(), values.end(), std::default_random_engine{7});

    auto v = SORTED_SET_T<int>{values.begin(), values.end()};
    CHECK(v.size() == 10000u);
    REQUIRE(v.impl().check_tree());
    CHECK(v.front() == 0);
    CHECK(v.back() == 9999);

    auto n    = std::size_t{};
    auto last = -1;
    for (auto&& x : v) {
        CHECK(x > last);
        last = x;
        ++n;
    }
    CHECK(n == v.size());
}
