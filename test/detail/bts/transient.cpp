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

#include <functional>
#include <map>
#include <random>
#include <stdexcept>
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

using tracked_small = btree<tracked,
                            tracked_key_fn,
                            std::less<int>,
                            immer::default_memory_policy,
                            2u,
                            2u>;

struct project_second
{
    template <typename P>
    const typename P::second_type& operator()(const P& p) const
    {
        return p.second;
    }
};

struct default_int
{
    int operator()() const { return 0; }
};

template <typename V>
struct combine_kv
{
    template <typename K, typename U>
    V operator()(K&& k, U&& v) const
    {
        return V{std::forward<K>(k), std::forward<U>(v)};
    }
};

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

} // namespace

TEST_CASE("bts transient: add_mut edits owned nodes in place")
{
    auto e = pair_small::owner_t{};
    auto t = pair_small::empty();

    t.add_mut(e, {0, 0});
    auto r1 = t.root;
    {
        auto em = pair_small::empty();
        CHECK(r1 != em.root); // the shared empty leaf is never mutated
    }
    t.add_mut(e, {2, 0});
    CHECK(t.root == r1);
    t.add_mut(e, {4, 0});
    CHECK(t.root == r1);
    t.add_mut(e, {6, 0});
    CHECK(t.root == r1); // in-place inserts while the leaf has room
    REQUIRE(t.check_tree());
    CHECK(t.size == 4u);
    CHECK(t.depth == 0u);

    t.add_mut(e, {8, 0}); // overflows the leaf, the root grows
    CHECK(t.root != r1);
    CHECK(t.depth == 1u);
    REQUIRE(t.check_tree());

    auto r2 = t.root;
    t.add_mut(e, {10, 0});
    CHECK(t.root == r2); // the inner root is edited in place
    REQUIRE(t.check_tree());
    CHECK(t.size == 6u);
}

TEST_CASE("bts transient: mixed tape against an oracle")
{
    auto run = [](auto tree, int universe, int n_ops, unsigned seed) {
        using tree_t = decltype(tree);
        auto e       = typename tree_t::owner_t{};
        auto engine  = std::default_random_engine{seed};
        auto key_gen = std::uniform_int_distribution<int>{0, universe - 1};
        auto op_gen  = std::uniform_int_distribution<int>{0, 99};
        auto oracle  = std::map<int, int>{};
        for (auto op = 0; op < n_ops; ++op) {
            auto k  = key_gen(engine) * 2;
            auto oc = op_gen(engine);
            if (oc < 50) {
                tree.add_mut(e, {k, op});
                oracle[k] = op;
            } else if (oc < 80) {
                tree.sub_mut(e, k);
                oracle.erase(k);
            } else {
                auto old = oracle.count(k) ? oracle[k] : 0;
                tree.template update_mut<project_second,
                                         default_int,
                                         combine_kv<std::pair<int, int>>>(
                    e, k, [](int x) { return x + 7; });
                oracle[k] = old + 7;
            }
            REQUIRE(tree.check_tree());
            REQUIRE(tree.size == oracle.size());
        }
        check_matches(tree, oracle);
    };
    run(pair_small::empty(), 150, 3000, 42);
    run(pair_set::empty(), 2000, 6000, 1984);
}

TEST_CASE("bts transient: frozen copies are preserved")
{
    auto e      = pair_small::owner_t{};
    auto t      = pair_small::empty();
    auto oracle = std::map<int, int>{};
    auto snaps  = std::vector<std::pair<pair_small, std::map<int, int>>>{};

    auto engine  = std::default_random_engine{7};
    auto key_gen = std::uniform_int_distribution<int>{0, 99};
    auto op_gen  = std::uniform_int_distribution<int>{0, 99};
    for (auto op = 0; op < 2000; ++op) {
        auto k = key_gen(engine) * 2;
        if (op_gen(engine) < 60) {
            t.add_mut(e, {k, op});
            oracle[k] = op;
        } else {
            t.sub_mut(e, k);
            oracle.erase(k);
        }
        REQUIRE(t.check_tree());
        if (op % 200 == 0)
            snaps.emplace_back(t, oracle); // freeze a copy mid-batch
    }
    check_matches(t, oracle);
    for (auto& s : snaps)
        check_matches(s.first, s.second);
}

TEST_CASE("bts transient: owned fast paths never copy values")
{
    REQUIRE(tracked::instances() == 0);
    {
        auto e = tracked_small::owner_t{};
        auto t = tracked_small::empty();

        // while the tree is uniquely owned and no node overflows or
        // underflows, insertion and erasure move values but never
        // copy them
        tracked::copies_until_throw() = 0;
        t.add_mut(e, tracked{0});
        t.add_mut(e, tracked{2});
        t.add_mut(e, tracked{4});
        t.add_mut(e, tracked{6});
        REQUIRE(t.check_tree());
        CHECK(t.size == 4u);

        t.sub_mut(e, 4);
        REQUIRE(t.check_tree());
        CHECK(t.size == 3u);
        tracked::copies_until_throw() = -1;

        t.add_mut(e, tracked{4});
        t.add_mut(e, tracked{8}); // overflow: this one copies
        REQUIRE(t.check_tree());
        CHECK(t.size == 5u);
    }
    CHECK(tracked::instances() == 0);
}

#ifndef IMMER_NO_EXCEPTIONS

TEST_CASE("bts transient: exception safety with shared structure")
{
    for (auto base_n : {5, 21, 85}) {
        auto e    = tracked_small::owner_t{};
        auto base = tracked_small::empty();
        for (auto i = 0; i < base_n; ++i)
            base.add_mut(e, tracked{2 * i});
        REQUIRE(base.check_tree());

        auto frozen = base; // sharing forbids in-place edits

        for (auto key : {-1, base_n | 1, 2 * base_n + 1, 2 * (base_n / 2)}) {
            auto before   = tracked::instances();
            auto existing = find_ptr(base, key) != nullptr;
            for (auto countdown = 0;; ++countdown) {
                REQUIRE(countdown < 10000);
                tracked::copies_until_throw() = countdown;
                auto done                     = false;
                try {
                    base.add_mut(e, tracked{key});
                    tracked::copies_until_throw() = -1;
                    REQUIRE(base.check_tree());
                    REQUIRE(base.size == frozen.size + (existing ? 0u : 1u));
                    done = true;
                } catch (const std::runtime_error&) {
                    tracked::copies_until_throw() = -1;
                    REQUIRE(tracked::instances() == before);
                    REQUIRE(base.check_tree());
                    REQUIRE(frozen.check_tree());
                    REQUIRE(base.size == frozen.size);
                }
                if (done)
                    break;
            }
            tracked::copies_until_throw() = -1;
            // undo for the next round
            base.sub_mut(e, key);
            if (existing)
                base.add_mut(e, tracked{key});
            REQUIRE(base.size == frozen.size);
        }

        REQUIRE(frozen.check_tree());
        REQUIRE(frozen.size == static_cast<size_t>(base_n));
        for (auto i = 0; i < base_n; ++i)
            REQUIRE(find_ptr(frozen, 2 * i) != nullptr);
    }
    CHECK(tracked::instances() == 0);
}

#endif // IMMER_NO_EXCEPTIONS
