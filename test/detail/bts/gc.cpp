//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include <immer/detail/bts/btree.hpp>
#include <immer/heap/gc_heap.hpp>
#include <immer/memory_policy.hpp>
#include <immer/refcount/no_refcount_policy.hpp>

#include "test/detail/bts/util.hpp"

#include <catch2/catch_test_macros.hpp>

#include <functional>
#include <map>
#include <random>
#include <utility>
#include <vector>

using namespace immer::detail::bts;
using namespace btstest;

namespace {

using gc_memory = immer::memory_policy<immer::heap_policy<immer::gc_heap>,
                                       immer::no_refcount_policy,
                                       immer::default_lock_policy,
                                       immer::gc_transience_policy,
                                       false>;

using pair_gc =
    btree<std::pair<int, int>, first_fn, std::less<int>, gc_memory, 2u, 2u>;

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

TEST_CASE("bts gc: edit tokens enable in-place mutation")
{
    // without reference counts, in-place mutation can only be
    // justified by the ownee marks
    auto e = pair_gc::owner_t{};
    auto t = pair_gc::empty();
    t.add_mut(e, {0, 0});
    auto r1 = t.root;
    t.add_mut(e, {2, 0});
    CHECK(t.root == r1);
    t.add_mut(e, {4, 0});
    t.add_mut(e, {6, 0});
    CHECK(t.root == r1);
    REQUIRE(t.check_tree());
    CHECK(t.size == 4u);
}

TEST_CASE("bts gc: erase rebalancing results stay owned")
{
    auto e = pair_gc::owner_t{};
    auto t = pair_gc::empty();
    for (auto k : {0, 2, 4, 6, 8})
        t.add_mut(e, {k, k});
    REQUIRE(t.depth == 1u);

    // erasing into an underflow merges the leaves and collapses the
    // root; the merged leaf must remain owned by the edit
    t.sub_mut(e, 8);
    REQUIRE(t.depth == 0u);
    REQUIRE(t.check_tree());
    CHECK(t.size == 4u);

    auto r = t.root;
    t.sub_mut(e, 6); // no underflow: edited in place
    CHECK(t.root == r);
    REQUIRE(t.check_tree());
    CHECK(t.size == 3u);
}

TEST_CASE("bts gc: refreshing the owner protects shared structure")
{
    auto e      = pair_gc::owner_t{};
    auto t      = pair_gc::empty();
    auto oracle = std::map<int, int>{};
    for (auto i = 0; i < 20; ++i) {
        t.add_mut(e, {i * 2, i});
        oracle[i * 2] = i;
    }
    auto frozen = t; // no refcounts: sharing is invisible to the nodes

    // this is what persistent() does on the transient owner
    e = pair_gc::owner_t{};

    for (auto i = 0; i < 20; ++i)
        t.add_mut(e, {i * 2 + 1, -i});
    REQUIRE(t.check_tree());
    REQUIRE(frozen.check_tree());
    CHECK(t.size == 40u);
    CHECK(t.root != frozen.root);
    check_matches(frozen, oracle);
}

TEST_CASE("bts gc: copying an owner invalidates both copies")
{
    auto e = pair_gc::owner_t{};
    auto t = pair_gc::empty();
    t.add_mut(e, {0, 0});
    t.add_mut(e, {2, 0});
    auto r1 = t.root;

    auto e2 = e; // both tokens are refreshed by the copy
    t.add_mut(e, {4, 0});
    CHECK(t.root != r1);
    REQUIRE(t.check_tree());
    CHECK(t.size == 3u);

    auto r2 = t.root;
    t.add_mut(e2, {6, 0});
    CHECK(t.root != r2);
    REQUIRE(t.check_tree());
}

TEST_CASE("bts gc: mixed tape against an oracle")
{
    auto e       = pair_gc::owner_t{};
    auto t       = pair_gc::empty();
    auto oracle  = std::map<int, int>{};
    auto engine  = std::default_random_engine{42};
    auto key_gen = std::uniform_int_distribution<int>{0, 119};
    auto op_gen  = std::uniform_int_distribution<int>{0, 99};
    for (auto op = 0; op < 1500; ++op) {
        auto k  = key_gen(engine) * 2;
        auto oc = op_gen(engine);
        if (oc < 50) {
            t.add_mut(e, {k, op});
            oracle[k] = op;
        } else if (oc < 80) {
            t.sub_mut(e, k);
            oracle.erase(k);
        } else {
            auto old = oracle.count(k) ? oracle[k] : 0;
            t.template update_mut<project_second,
                                  default_int,
                                  combine_kv<std::pair<int, int>>>(
                e, k, [](int x) { return x + 7; });
            oracle[k] = old + 7;
        }
        REQUIRE(t.check_tree());
        REQUIRE(t.size == oracle.size());
    }
    check_matches(t, oracle);
}
