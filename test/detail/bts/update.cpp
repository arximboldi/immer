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

using pair_small = btree<std::pair<int, int>,
                         first_fn,
                         std::less<int>,
                         immer::default_memory_policy,
                         2u,
                         2u>;

using tracked_pairs = btree<std::pair<int, tracked>,
                            first_fn,
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

template <typename Tree, typename Key, typename Fn>
Tree update_tree(const Tree& t, const Key& k, Fn&& fn)
{
    return t.template update<project_second,
                             default_int,
                             combine_kv<typename Tree::value_t>>(
        k, std::forward<Fn>(fn));
}

template <typename Tree, typename Key, typename Fn>
Tree update_tree_if_exists(const Tree& t, const Key& k, Fn&& fn)
{
    return t.template update_if_exists<project_second,
                                       combine_kv<typename Tree::value_t>>(
        k, std::forward<Fn>(fn));
}

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

TEST_CASE("bts update: transforms an existing value")
{
    auto t = pair_small::empty();
    for (auto k : spread_keys(100, 2))
        t = t.add({k, k});

    auto t2 = update_tree(t, 40, [](int v) { return v + 1000; });
    REQUIRE(t2.check_tree());
    CHECK(t2.size == t.size);
    CHECK(find_ptr(t2, 40)->second == 1040);
    CHECK(find_ptr(t, 40)->second == 40);
    CHECK(find_ptr(t2, 42)->second == 42);
}

TEST_CASE("bts update: inserts the transformed default when missing")
{
    auto t = pair_small::empty();
    for (auto k : spread_keys(100, 2))
        t = t.add({k, k});

    auto t2 = update_tree(t, 41, [](int v) {
        CHECK(v == 0);
        return 7;
    });
    REQUIRE(t2.check_tree());
    CHECK(t2.size == t.size + 1u);
    CHECK(find_ptr(t2, 41)->second == 7);
    CHECK(find_ptr(t, 41) == nullptr);
}

TEST_CASE("bts update: update_if_exists on a missing key is identity")
{
    auto t = pair_small::empty();
    for (auto k : spread_keys(100, 2))
        t = t.add({k, k});

    auto old_root = t.root;
    auto t2       = update_tree_if_exists(t, 41, [](int v) { return v + 1; });
    CHECK(t2.root == old_root);
    CHECK(t2.size == t.size);

    auto t3 = update_tree_if_exists(t, 40, [](int v) { return v + 1; });
    REQUIRE(t3.check_tree());
    CHECK(t3.size == t.size);
    CHECK(find_ptr(t3, 40)->second == 41);
    CHECK(find_ptr(t, 40)->second == 40);
}

TEST_CASE("bts update: random tape against an oracle")
{
    auto engine  = std::default_random_engine{99};
    auto key_gen = std::uniform_int_distribution<int>{0, 149};
    auto op_gen  = std::uniform_int_distribution<int>{0, 99};
    auto t       = pair_small::empty();
    auto oracle  = std::map<int, int>{};
    auto fn      = [](int v) { return v * 2 + 1; };
    for (auto op = 0; op < 1500; ++op) {
        auto k  = key_gen(engine) * 2;
        auto oc = op_gen(engine);
        if (oc < 40) {
            t         = t.add({k, op});
            oracle[k] = op;
        } else if (oc < 65) {
            t = t.sub(k);
            oracle.erase(k);
        } else if (oc < 85) {
            auto old  = oracle.count(k) ? oracle[k] : 0;
            t         = update_tree(t, k, fn);
            oracle[k] = fn(old);
        } else {
            t = update_tree_if_exists(t, k, fn);
            if (oracle.count(k))
                oracle[k] = fn(oracle[k]);
        }
        REQUIRE(t.check_tree());
        REQUIRE(t.size == oracle.size());
    }
    check_matches(t, oracle);
}

#ifndef IMMER_NO_EXCEPTIONS

TEST_CASE("bts update: strong exception safety")
{
    for (auto base_n : {5, 21, 85}) {
        auto base = tracked_pairs::empty();
        for (auto i = 0; i < base_n; ++i)
            base = base.add({2 * i, tracked{i}});
        REQUIRE(base.check_tree());

        auto fn = [](const tracked& v) { return tracked{v.value + 1000}; };
        auto try_keys =
            std::vector<int>{0, 2 * (base_n / 2), 2 * (base_n - 1), -1};
        for (auto key : try_keys) {
            auto before   = tracked::instances();
            auto existing = find_ptr(base, key) != nullptr;
            for (auto countdown = 0;; ++countdown) {
                REQUIRE(countdown < 10000);
                tracked::copies_until_throw() = countdown;
                auto done                     = false;
                try {
                    auto t2                       = update_tree(base, key, fn);
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

#endif // IMMER_NO_EXCEPTIONS
