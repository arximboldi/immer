//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#ifndef SORTED_MAP_T
#error "define the map template to use in SORTED_MAP_T"
#include <immer/sorted_map.hpp>
#define SORTED_MAP_T ::immer::sorted_map
#endif

#include "test/util.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <functional>
#include <map>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

IMMER_RANGES_CHECK(
    std::ranges::bidirectional_range<SORTED_MAP_T<std::string, std::string>>);

namespace {

template <typename Map, typename Oracle>
void check_equals_oracle(const Map& m, const Oracle& oracle)
{
    REQUIRE(m.impl().check_tree());
    REQUIRE(m.size() == oracle.size());
    REQUIRE(std::equal(m.begin(),
                       m.end(),
                       oracle.begin(),
                       oracle.end(),
                       [](auto&& a, auto&& b) {
                           return a.first == b.first && a.second == b.second;
                       }));
}

} // namespace

TEST_CASE("sorted_map: instantiation and basic queries")
{
    auto v = SORTED_MAP_T<int, int>{};
    CHECK(v.size() == 0u);
    CHECK(v.empty());
    CHECK(v.begin() == v.end());
    CHECK(v.count(42) == 0u);
    CHECK(v.find(42) == nullptr);
    CHECK(v.identity() == SORTED_MAP_T<int, int>{}.identity());
}

TEST_CASE("sorted_map: constructors")
{
    auto v = SORTED_MAP_T<int, int>{{1, 10}, {3, 30}, {2, 20}, {1, 11}};
    CHECK(v.size() == 3u);
    CHECK(v.at(1) == 11); // the last repeated key wins
    CHECK(v.at(2) == 20);
    CHECK(v.at(3) == 30);

    auto values = std::vector<std::pair<int, int>>{{5, 50}, {4, 40}};
    auto u      = SORTED_MAP_T<int, int>{values.begin(), values.end()};
    CHECK(u.size() == 2u);
    CHECK(u.at(4) == 40);
    CHECK(u.at(5) == 50);
}

TEST_CASE("sorted_map: insert, set, find, count, at and operator[]")
{
    auto v = SORTED_MAP_T<int, int>{};
    for (auto i = 0; i < 100; ++i)
        v = v.insert({i * 2, i});
    CHECK(v.size() == 100u);
    CHECK(v.count(50) == 1u);
    CHECK(v.count(51) == 0u);
    REQUIRE(v.find(50) != nullptr);
    CHECK(*v.find(50) == 25);
    CHECK(v.find(51) == nullptr);
    CHECK(v.at(50) == 25);
    CHECK(v[50] == 25);
    CHECK(v[51] == 0); // default value for missing keys

    auto u = v.set(50, 1000);
    CHECK(u.at(50) == 1000);
    CHECK(v.at(50) == 25);
    CHECK(u.size() == v.size());

#ifndef IMMER_NO_EXCEPTIONS
    CHECK_THROWS_AS(v.at(51), std::out_of_range);
#endif
}

TEST_CASE("sorted_map: iteration is sorted by key")
{
    auto keys = std::vector<int>{};
    for (auto i = 0; i < 500; ++i)
        keys.push_back(i * 3);
    auto shuffled = keys;
    std::shuffle(
        shuffled.begin(), shuffled.end(), std::default_random_engine{42});

    auto v = SORTED_MAP_T<int, int>{};
    for (auto k : shuffled)
        v = v.set(k, k + 1);

    auto seen = std::vector<int>{};
    for (auto&& kv : v) {
        CHECK(kv.second == kv.first + 1);
        seen.push_back(kv.first);
    }
    CHECK(seen == keys);

    auto rseen = std::vector<int>{};
    for (auto it = v.rbegin(); it != v.rend(); ++it)
        rseen.push_back(it->first);
    std::reverse(rseen.begin(), rseen.end());
    CHECK(rseen == keys);
}

TEST_CASE("sorted_map: front and back")
{
    auto v = SORTED_MAP_T<int, int>{{5, 50}, {1, 10}, {3, 30}};
    CHECK(v.front().first == 1);
    CHECK(v.front().second == 10);
    CHECK(v.back().first == 5);
    CHECK(v.back().second == 50);
}

TEST_CASE("sorted_map: lower_bound and upper_bound")
{
    auto v = SORTED_MAP_T<int, int>{};
    for (auto i = 0; i < 100; ++i)
        v = v.set(i * 10, i);

    CHECK(v.lower_bound(50)->first == 50);
    CHECK(v.lower_bound(51)->first == 60);
    CHECK(v.upper_bound(50)->first == 60);
    CHECK(v.lower_bound(-1)->first == 0);
    CHECK(v.upper_bound(990) == v.end());
    CHECK(v.lower_bound(991) == v.end());

    auto sum = 0;
    for (auto it = v.lower_bound(100), e = v.upper_bound(150); it != e; ++it)
        sum += it->first;
    CHECK(sum == 100 + 110 + 120 + 130 + 140 + 150);
}

TEST_CASE("sorted_map: erase")
{
    auto v = SORTED_MAP_T<int, int>{};
    for (auto i = 0; i < 200; ++i)
        v = v.set(i, i);

    auto u = v.erase(100);
    CHECK(u.size() == 199u);
    CHECK(u.find(100) == nullptr);
    CHECK(v.find(100) != nullptr);

    CHECK(v.erase(1000).identity() == v.identity());

    auto w = v;
    for (auto i = 0; i < 200; ++i)
        w = w.erase(i);
    CHECK(w.empty());
    CHECK(w.begin() == w.end());
}

TEST_CASE("sorted_map: update and update_if_exists")
{
    auto v = SORTED_MAP_T<int, int>{{1, 10}, {2, 20}};

    auto u = v.update(1, [](int x) { return x + 1; });
    CHECK(u.at(1) == 11);
    CHECK(v.at(1) == 10);

    auto w = v.update(3, [](int x) { return x + 1; });
    CHECK(w.size() == 3u);
    CHECK(w.at(3) == 1);

    auto s = v.update_if_exists(3, [](int x) { return x + 1; });
    CHECK(s.identity() == v.identity());

    auto z = v.update_if_exists(2, [](int x) { return x * 2; });
    CHECK(z.at(2) == 40);
    CHECK(z.size() == 2u);
}

TEST_CASE("sorted_map: equality")
{
    auto a = SORTED_MAP_T<int, int>{{1, 10}, {2, 20}, {3, 30}};
    auto b = SORTED_MAP_T<int, int>{};
    b      = b.set(3, 30).set(1, 10).set(2, 20);
    CHECK(a == b);
    CHECK(!(a != b));
    CHECK(a != b.set(2, 21));
    CHECK(a != b.erase(2));
    CHECK(a == a.erase(42));

    auto c = a;
    CHECK(c == a);
    CHECK(c.identity() == a.identity());
}

TEST_CASE("sorted_map: random operations against std::map")
{
    auto engine  = std::default_random_engine{2026};
    auto key_gen = std::uniform_int_distribution<int>{0, 254};
    auto op_gen  = std::uniform_int_distribution<int>{0, 99};

    auto v      = SORTED_MAP_T<int, int>{};
    auto oracle = std::map<int, int>{};
    for (auto op = 0; op < 2000; ++op) {
        auto k  = key_gen(engine);
        auto oc = op_gen(engine);
        if (oc < 40) {
            v         = v.set(k, op);
            oracle[k] = op;
        } else if (oc < 65) {
            v = v.erase(k);
            oracle.erase(k);
        } else if (oc < 80) {
            auto old  = oracle.count(k) ? oracle[k] : 0;
            v         = v.update(k, [](int x) { return x + 7; });
            oracle[k] = old + 7;
        } else {
            v = v.update_if_exists(k, [](int x) { return x - 3; });
            if (oracle.count(k))
                oracle[k] -= 3;
        }
        REQUIRE(v.size() == oracle.size());
        if (op % 100 == 0)
            check_equals_oracle(v, oracle);
    }
    check_equals_oracle(v, oracle);
}

TEST_CASE("sorted_map: string keys")
{
    auto v = SORTED_MAP_T<std::string, std::string>{};
    v      = v.set("hello", "world").set("alpha", "beta").set("zeta", "eta");
    CHECK(v.size() == 3u);
    CHECK(v.at("hello") == "world");
    CHECK(v.front().first == "alpha");
    CHECK(v.back().first == "zeta");
    CHECK(v.begin()->first == "alpha");
}

TEST_CASE("sorted_map: transparent comparator lookups")
{
    auto v = SORTED_MAP_T<std::string, int, std::less<>>{};
    for (auto i = 0; i < 100; ++i)
        v = v.set("key" + std::to_string(i), i);

    CHECK(v.count("key42") == 1u);
    CHECK(v.count("nope") == 0u);
    REQUIRE(v.find("key42") != nullptr);
    CHECK(*v.find("key42") == 42);
    CHECK(v.at("key42") == 42);
    CHECK(v["key42"] == 42);
    CHECK(v.lower_bound("key42")->second == 42);
    CHECK(v.upper_bound("key99") == v.end());
}

TEST_CASE("sorted_map: bigger map")
{
    auto values = std::vector<std::pair<int, int>>{};
    for (auto i = 0; i < 10000; ++i)
        values.push_back({i, i * 2});
    auto shuffled = values;
    std::shuffle(
        shuffled.begin(), shuffled.end(), std::default_random_engine{7});

    auto v = SORTED_MAP_T<int, int>{shuffled.begin(), shuffled.end()};
    CHECK(v.size() == 10000u);
    REQUIRE(v.impl().check_tree());
    CHECK(v.at(5000) == 10000);
    CHECK(v.front().first == 0);
    CHECK(v.back().first == 9999);

    auto n    = std::size_t{};
    auto last = -1;
    for (auto&& kv : v) {
        CHECK(kv.first > last);
        last = kv.first;
        ++n;
    }
    CHECK(n == v.size());
}
