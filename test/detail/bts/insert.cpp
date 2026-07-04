//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include <immer/detail/bts/btree.hpp>
#include <immer/memory_policy.hpp>

#include "test/detail/bts/util.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <functional>
#include <map>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace immer::detail::bts;
using namespace btstest;

namespace {

struct tracked_key_fn
{
    int operator()(const tracked& x) const { return x.value; }
};

using pair_small = btree<std::pair<int, int>,
                         first_fn,
                         std::less<int>,
                         immer::default_memory_policy,
                         2u,
                         2u>;

using pair_set = btree<std::pair<int, int>,
                       first_fn,
                       std::less<int>,
                       immer::default_memory_policy,
                       5u,
                       5u>;

using str_small = btree<std::pair<std::string, int>,
                        first_fn,
                        std::less<std::string>,
                        immer::default_memory_policy,
                        2u,
                        2u>;

using tracked_small = btree<tracked,
                            tracked_key_fn,
                            std::less<int>,
                            immer::default_memory_policy,
                            2u,
                            2u>;

using thr_map = btree<std::pair<throwing_key, int>,
                      first_fn,
                      std::less<throwing_key>,
                      immer::default_memory_policy,
                      2u,
                      2u>;

template <typename Tree, typename Oracle>
void check_matches(const Tree& t, const Oracle& expected)
{
    REQUIRE(t.check_tree());
    REQUIRE(t.size == expected.size());
    for (auto& kv : expected) {
        auto p = find_ptr(t, kv.first);
        REQUIRE(p != nullptr);
        REQUIRE(p->second == kv.second);
    }
}

template <typename Tree>
void check_insert_pattern(const std::vector<int>& keys)
{
    auto t      = Tree::empty();
    auto oracle = std::map<int, int>{};
    auto op     = 0;
    for (auto k : keys) {
        t         = t.add({k, op});
        oracle[k] = op;
        ++op;
        REQUIRE(t.check_tree());
        REQUIRE(t.size == oracle.size());
    }
    check_matches(t, oracle);
}

template <typename Tree>
void check_random_tape(int universe, int n_ops, int snap_every, unsigned seed)
{
    auto engine = std::default_random_engine{seed};
    auto dist   = std::uniform_int_distribution<int>{0, universe - 1};
    auto snaps  = std::vector<std::pair<Tree, std::map<int, int>>>{};
    auto t      = Tree::empty();
    auto oracle = std::map<int, int>{};
    for (auto op = 0; op < n_ops; ++op) {
        auto k    = dist(engine) * 2;
        t         = t.add({k, op});
        oracle[k] = op;
        REQUIRE(t.check_tree());
        REQUIRE(t.size == oracle.size());
        if (op % snap_every == 0)
            snaps.emplace_back(t, oracle);
    }
    check_matches(t, oracle);
    for (auto k = 1; k < 20; k += 2)
        REQUIRE(find_ptr(t, k) == nullptr);
    for (auto& s : snaps)
        check_matches(s.first, s.second);
}

} // namespace

TEST_CASE("bts insert: from empty and repeated single key")
{
    auto t = pair_small::empty();
    t      = t.add({5, 0});
    CHECK(t.size == 1u);
    CHECK(t.depth == 0u);
    for (auto i = 1; i < 100; ++i) {
        t = t.add({5, i});
        REQUIRE(t.size == 1u);
        REQUIRE(t.check_tree());
    }
    auto p = find_ptr(t, 5);
    REQUIRE(p != nullptr);
    CHECK(p->second == 99);
}

TEST_CASE("bts insert: ascending, descending and shuffled")
{
    auto asc = spread_keys(300, 3);
    check_insert_pattern<pair_small>(asc);

    auto desc = asc;
    std::reverse(desc.begin(), desc.end());
    check_insert_pattern<pair_small>(desc);

    auto shuf = asc;
    std::shuffle(shuf.begin(), shuf.end(), std::default_random_engine{42});
    check_insert_pattern<pair_small>(shuf);

    auto asc_big = spread_keys(1200, 2);
    check_insert_pattern<pair_set>(asc_big);

    auto shuf_big = asc_big;
    std::shuffle(
        shuf_big.begin(), shuf_big.end(), std::default_random_engine{17});
    check_insert_pattern<pair_set>(shuf_big);
}

TEST_CASE("bts insert: replaces on equal key")
{
    auto t      = pair_small::empty();
    auto oracle = std::map<int, int>{};
    for (auto round = 0; round < 3; ++round) {
        for (auto k : spread_keys(60, 2)) {
            auto v    = round * 1000 + k;
            t         = t.add({k, v});
            oracle[k] = v;
            REQUIRE(t.check_tree());
        }
        REQUIRE(t.size == 60u);
        check_matches(t, oracle);
    }
}

TEST_CASE("bts insert: random tape with persistent snapshots")
{
    check_random_tape<pair_small>(250, 2500, 250, 42);
    check_random_tape<pair_set>(3000, 6000, 500, 1984);
}

TEST_CASE("bts insert: string keys")
{
    auto engine = std::default_random_engine{7};
    auto dist   = std::uniform_int_distribution<int>{0, 300};
    auto t      = str_small::empty();
    auto oracle = std::map<std::string, int>{};
    for (auto op = 0; op < 800; ++op) {
        auto k    = pad(dist(engine) * 2);
        t         = t.add({k, op});
        oracle[k] = op;
        REQUIRE(t.check_tree());
    }
    check_matches(t, oracle);
    CHECK(find_ptr(t, pad(1)) == nullptr);
    CHECK(find_ptr(t, pad(41)) == nullptr);
}

TEST_CASE("bts insert: structural sharing between versions")
{
    auto t      = pair_set::empty();
    auto oracle = std::map<int, int>{};
    for (auto k : spread_keys(2000, 2)) {
        t         = t.add({k, k});
        oracle[k] = k;
    }
    REQUIRE(t.check_tree());
    REQUIRE(t.depth >= 2u);

    SECTION("inserting at the front shares the rest of the tree")
    {
        auto t2 = t.add({-1, -1});
        CHECK(t2.root != t.root);
        REQUIRE(t2.depth == t.depth);
        auto last_t  = t.root->children()[t.root->count() - 1u];
        auto last_t2 = t2.root->children()[t2.root->count() - 1u];
        CHECK(last_t == last_t2);
        CHECK(!pair_set::node_t::refs(last_t).unique());
        check_matches(t, oracle);
        auto oracle2 = oracle;
        oracle2[-1]  = -1;
        check_matches(t2, oracle2);
    }

    SECTION("replacing does not grow the tree and keeps the old version")
    {
        auto t3 = t.add({0, 999});
        CHECK(t3.size == t.size);
        CHECK(t3.depth == t.depth);
        CHECK(find_ptr(t, 0)->second == 0);
        CHECK(find_ptr(t3, 0)->second == 999);
        check_matches(t, oracle);
    }
}

#ifndef IMMER_NO_EXCEPTIONS

TEST_CASE("bts insert: strong exception safety with throwing values")
{
    for (auto base_n : {4, 20, 84}) {
        auto base = tracked_small::empty();
        for (auto i = 0; i < base_n; ++i) {
            base = base.add(tracked{2 * i});
        }
        REQUIRE(base.check_tree());
        REQUIRE(base.size == static_cast<size_t>(base_n));

        auto try_keys =
            std::vector<int>{-1, base_n | 1, 2 * base_n + 1, 2 * (base_n / 2)};
        for (auto key : try_keys) {
            auto before   = tracked::instances();
            auto existing = find_ptr(base, key) != nullptr;
            for (auto countdown = 0;; ++countdown) {
                REQUIRE(countdown < 10000);
                tracked::copies_until_throw() = countdown;
                auto done                     = false;
                try {
                    auto t2                       = base.add(tracked{key});
                    tracked::copies_until_throw() = -1;
                    REQUIRE(t2.check_tree());
                    REQUIRE(t2.size == base.size + (existing ? 0u : 1u));
                    REQUIRE(find_ptr(t2, key) != nullptr);
                    done = true;
                } catch (const std::runtime_error&) {
                    tracked::copies_until_throw() = -1;
                    REQUIRE(tracked::instances() == before);
                    REQUIRE(base.check_tree());
                    REQUIRE(base.size == static_cast<size_t>(base_n));
                }
                if (done)
                    break;
            }
            tracked::copies_until_throw() = -1;
            REQUIRE(tracked::instances() == before);
        }
    }
    CHECK(tracked::instances() == 0);
}

TEST_CASE("bts insert: strong exception safety with throwing keys")
{
    for (auto base_n : {4, 20, 84}) {
        auto base   = thr_map::empty();
        auto oracle = std::map<std::string, int>{};
        for (auto i = 0; i < base_n; ++i) {
            base               = base.add({throwing_key{pad(2 * i)}, i});
            oracle[pad(2 * i)] = i;
        }
        REQUIRE(base.check_tree());

        auto verify_base = [&] {
            REQUIRE(base.check_tree());
            REQUIRE(base.size == oracle.size());
            for (auto& kv : oracle) {
                auto p = find_ptr(base, throwing_key{kv.first});
                REQUIRE(p != nullptr);
                REQUIRE(p->second == kv.second);
            }
        };

        auto try_keys = std::vector<std::string>{
            pad(1), pad(base_n | 1), pad(2 * base_n + 1), pad(0)};
        for (auto& key : try_keys) {
            auto existing = find_ptr(base, throwing_key{key}) != nullptr;
            for (auto countdown = 0;; ++countdown) {
                REQUIRE(countdown < 10000);
                throwing_key::copies_until_throw() = countdown;
                auto done                          = false;
                try {
                    auto t2 = base.add({throwing_key{key}, -1});
                    throwing_key::copies_until_throw() = -1;
                    REQUIRE(t2.check_tree());
                    REQUIRE(t2.size == base.size + (existing ? 0u : 1u));
                    REQUIRE(find_ptr(t2, throwing_key{key}) != nullptr);
                    done = true;
                } catch (const std::runtime_error&) {
                    throwing_key::copies_until_throw() = -1;
                    verify_base();
                }
                if (done)
                    break;
            }
            throwing_key::copies_until_throw() = -1;
        }
    }
}

#endif // IMMER_NO_EXCEPTIONS
