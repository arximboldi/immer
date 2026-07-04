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
void check_erase_all(const std::vector<int>& order)
{
    auto t = Tree::empty();
    for (auto k : order)
        t = t.add({k, k});
    REQUIRE(t.check_tree());
    auto left = order.size();
    for (auto k : order) {
        t = t.sub(k);
        --left;
        REQUIRE(t.check_tree());
        REQUIRE(t.size == left);
        REQUIRE(find_ptr(t, k) == nullptr);
    }
    REQUIRE(t.size == 0u);
    REQUIRE(t.depth == 0u);
    REQUIRE(t.root == Tree::empty().root);
}

template <typename Tree>
void check_mixed_tape(
    int universe, int n_ops, int erase_pct, int snap_every, unsigned seed)
{
    auto engine  = std::default_random_engine{seed};
    auto key_gen = std::uniform_int_distribution<int>{0, universe - 1};
    auto op_gen  = std::uniform_int_distribution<int>{0, 99};
    auto snaps   = std::vector<std::pair<Tree, std::map<int, int>>>{};
    auto t       = Tree::empty();
    auto oracle  = std::map<int, int>{};
    for (auto op = 0; op < n_ops; ++op) {
        auto k = key_gen(engine) * 2;
        if (op_gen(engine) < erase_pct) {
            auto present  = oracle.count(k) > 0;
            auto old_root = t.root;
            t             = t.sub(k);
            oracle.erase(k);
            if (!present)
                REQUIRE(t.root == old_root);
        } else {
            t         = t.add({k, op});
            oracle[k] = op;
        }
        REQUIRE(t.check_tree());
        REQUIRE(t.size == oracle.size());
        if (op % snap_every == 0)
            snaps.emplace_back(t, oracle);
    }
    check_matches(t, oracle);
    for (auto& s : snaps)
        check_matches(s.first, s.second);
}

} // namespace

TEST_CASE("bts erase: missing keys are identity")
{
    auto e = pair_small::empty();
    CHECK(e.sub(42).root == e.root);
    CHECK(e.sub(42).size == 0u);

    auto t = pair_small::empty();
    for (auto k : spread_keys(100, 2))
        t = t.add({k, k});
    auto old_root = t.root;
    CHECK(t.sub(1).root == old_root);
    CHECK(t.sub(-100).root == old_root);
    CHECK(t.sub(1000).root == old_root);
    CHECK(t.sub(1).size == t.size);
}

TEST_CASE("bts erase: erase all elements in various orders")
{
    auto keys = spread_keys(300, 2);
    check_erase_all<pair_small>(keys);

    auto desc = keys;
    std::reverse(desc.begin(), desc.end());
    check_erase_all<pair_small>(desc);

    auto shuf = keys;
    std::shuffle(shuf.begin(), shuf.end(), std::default_random_engine{42});
    check_erase_all<pair_small>(shuf);

    auto big = spread_keys(1200, 2);
    std::shuffle(big.begin(), big.end(), std::default_random_engine{17});
    check_erase_all<pair_set>(big);
}

TEST_CASE("bts erase: mixed random tapes with persistent snapshots")
{
    check_mixed_tape<pair_small>(120, 4000, 45, 400, 42);
    check_mixed_tape<pair_small>(60, 3000, 65, 300, 7);
    check_mixed_tape<pair_set>(2000, 8000, 45, 800, 1984);
}

TEST_CASE("bts erase: string keys mixed tape")
{
    auto engine  = std::default_random_engine{23};
    auto key_gen = std::uniform_int_distribution<int>{0, 200};
    auto op_gen  = std::uniform_int_distribution<int>{0, 99};
    auto t       = str_small::empty();
    auto oracle  = std::map<std::string, int>{};
    for (auto op = 0; op < 1500; ++op) {
        auto k = pad(key_gen(engine) * 2);
        if (op_gen(engine) < 40) {
            t = t.sub(k);
            oracle.erase(k);
        } else {
            t         = t.add({k, op});
            oracle[k] = op;
        }
        REQUIRE(t.check_tree());
        REQUIRE(t.size == oracle.size());
    }
    check_matches(t, oracle);
}

TEST_CASE("bts erase: root collapses as the tree shrinks")
{
    auto t = pair_small::empty();
    for (auto k : spread_keys(100, 1))
        t = t.add({k, k});
    REQUIRE(t.depth >= 2u);
    auto last_depth = t.depth;
    for (auto k : spread_keys(100, 1)) {
        t = t.sub(k);
        REQUIRE(t.check_tree());
        REQUIRE(t.depth <= last_depth);
        last_depth = t.depth;
    }
    CHECK(t.size == 0u);
    CHECK(t.depth == 0u);
}

TEST_CASE("bts erase: keeps old versions intact")
{
    auto t      = pair_set::empty();
    auto oracle = std::map<int, int>{};
    for (auto k : spread_keys(1000, 2)) {
        t         = t.add({k, k});
        oracle[k] = k;
    }
    auto t2 = t.sub(500);
    CHECK(find_ptr(t, 500) != nullptr);
    CHECK(find_ptr(t2, 500) == nullptr);
    CHECK(t2.size == t.size - 1u);
    check_matches(t, oracle);
    auto oracle2 = oracle;
    oracle2.erase(500);
    check_matches(t2, oracle2);
}

#ifndef IMMER_NO_EXCEPTIONS

TEST_CASE("bts erase: strong exception safety with throwing values")
{
    for (auto base_n : {5, 21, 85}) {
        auto base = tracked_small::empty();
        for (auto i = 0; i < base_n; ++i)
            base = base.add(tracked{2 * i});
        REQUIRE(base.check_tree());

        auto try_keys = std::vector<int>{0, 2 * (base_n / 2), 2 * (base_n - 1)};
        for (auto key : try_keys) {
            auto before = tracked::instances();
            for (auto countdown = 0;; ++countdown) {
                REQUIRE(countdown < 10000);
                tracked::copies_until_throw() = countdown;
                auto done                     = false;
                try {
                    auto t2                       = base.sub(key);
                    tracked::copies_until_throw() = -1;
                    REQUIRE(t2.check_tree());
                    REQUIRE(t2.size == base.size - 1u);
                    REQUIRE(find_ptr(t2, key) == nullptr);
                    done = true;
                } catch (const std::runtime_error&) {
                    tracked::copies_until_throw() = -1;
                    REQUIRE(tracked::instances() == before);
                    REQUIRE(base.check_tree());
                    REQUIRE(base.size == static_cast<size_t>(base_n));
                    REQUIRE(find_ptr(base, key) != nullptr);
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

TEST_CASE("bts erase: strong exception safety with throwing keys")
{
    for (auto base_n : {5, 21, 85}) {
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
            pad(0), pad(2 * (base_n / 2)), pad(2 * (base_n - 1))};
        for (auto& key : try_keys) {
            for (auto countdown = 0;; ++countdown) {
                REQUIRE(countdown < 10000);
                throwing_key::copies_until_throw() = countdown;
                auto done                          = false;
                try {
                    auto t2 = base.sub(throwing_key{key});
                    throwing_key::copies_until_throw() = -1;
                    REQUIRE(t2.check_tree());
                    REQUIRE(t2.size == base.size - 1u);
                    REQUIRE(find_ptr(t2, throwing_key{key}) == nullptr);
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
