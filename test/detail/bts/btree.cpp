//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#define IMMER_DEBUG_STATS 1

#include <immer/detail/bts/btree.hpp>
#include <immer/memory_policy.hpp>

#include "test/detail/bts/util.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <functional>
#include <string>
#include <utility>
#include <vector>

using namespace immer::detail::bts;
using namespace btstest;

namespace {

using set_tree = btree<int,
                       identity_fn,
                       std::less<int>,
                       immer::default_memory_policy,
                       5u,
                       5u>;

using small_tree = btree<int,
                         identity_fn,
                         std::less<int>,
                         immer::default_memory_policy,
                         2u,
                         2u>;

using map_tree = btree<std::pair<std::string, int>,
                       first_fn,
                       std::less<>,
                       immer::default_memory_policy,
                       2u,
                       2u>;

using tracked_tree = btree<std::pair<std::string, tracked>,
                           first_fn,
                           std::less<std::string>,
                           immer::default_memory_policy,
                           2u,
                           2u>;

using unsafe_memory = immer::memory_policy<immer::heap_policy<immer::cpp_heap>,
                                           immer::unsafe_refcount_policy,
                                           immer::no_lock_policy>;

using unsafe_tree =
    btree<int, identity_fn, std::less<int>, unsafe_memory, 3u, 3u>;

static_assert(branches<5u> == 32u, "");
static_assert(min_branches<5u> == 16u, "");
static_assert(max_depth<5u> == 9u, "");
static_assert(max_depth<2u> == 33u, "");

} // namespace

TEST_CASE("bts btree: empty tree")
{
    auto t = set_tree::empty();
    CHECK(t.size == 0u);
    CHECK(t.depth == 0u);
    CHECK(t.check_tree());
    CHECK(find_ptr(t, 42) == nullptr);

    CHECK(set_tree::empty().root == set_tree::empty().root);

    auto c = t;
    CHECK(c.root == t.root);
    auto m = std::move(c);
    CHECK(m.size == 0u);
    CHECK(m.check_tree());
}

TEST_CASE("bts btree: packed build and lookup, small branching factor")
{
    for (auto n : {1, 2, 3, 4, 5, 8, 16, 17, 64, 100, 257}) {
        auto keys = spread_keys(n, 3);
        auto t    = build_packed<small_tree>(keys);
        CHECK(t.size == static_cast<size_t>(n));
        REQUIRE(t.check_tree());
        for (auto k : keys) {
            auto p = find_ptr(t, k);
            REQUIRE(p != nullptr);
            CHECK(*p == k);
        }
        for (auto k : keys) {
            CHECK(find_ptr(t, k + 1) == nullptr);
            CHECK(find_ptr(t, k - 1) == nullptr);
        }
        CHECK(find_ptr(t, -100) == nullptr);
        CHECK(find_ptr(t, 3 * n + 100) == nullptr);
    }
}

TEST_CASE("bts btree: packed build and lookup, default branching factor")
{
    for (auto n : {33, 1000, 20000}) {
        auto keys = spread_keys(n, 2, 1);
        auto t    = build_packed<set_tree>(keys);
        CHECK(t.size == static_cast<size_t>(n));
        REQUIRE(t.check_tree());
        CHECK(t.depth >= (n > 32 ? 1u : 0u));
        for (auto k : keys) {
            auto p = find_ptr(t, k);
            REQUIRE(p != nullptr);
            CHECK(*p == k);
        }
        CHECK(find_ptr(t, 0) == nullptr);
        CHECK(find_ptr(t, 2) == nullptr);
        CHECK(find_ptr(t, 2 * n + 2) == nullptr);
    }
}

TEST_CASE("bts btree: copies share structure")
{
    auto t = build_packed<set_tree>(spread_keys(1000, 1));
    {
        auto c = t;
        CHECK(c.root == t.root);
        CHECK(!set_tree::node_t::refs(t.root).unique());
        CHECK(c.check_tree());
    }
    CHECK(set_tree::node_t::refs(t.root).unique());
    CHECK(t.check_tree());
}

TEST_CASE("bts btree: validator detects corruption")
{
    auto t = build_packed<small_tree>(spread_keys(64, 1));
    REQUIRE(t.depth == 2u);
    REQUIRE(t.check_tree());

    SECTION("out of order values in a leaf")
    {
        auto leaf = t.root->children()[0]->children()[0];
        std::swap(leaf->values()[0], leaf->values()[1]);
        CHECK(!t.check_tree());
        std::swap(leaf->values()[0], leaf->values()[1]);
        CHECK(t.check_tree());
    }

    SECTION("inconsistent cumulative sizes")
    {
        ++t.root->sizes()[0];
        CHECK(!t.check_tree());
        --t.root->sizes()[0];
        CHECK(t.check_tree());
    }

    SECTION("out of order separators")
    {
        std::swap(t.root->keys()[0], t.root->keys()[1]);
        CHECK(!t.check_tree());
        std::swap(t.root->keys()[0], t.root->keys()[1]);
        CHECK(t.check_tree());
    }

    SECTION("separator that does not partition its subtrees")
    {
        auto old          = t.root->keys()[0];
        t.root->keys()[0] = 0;
        CHECK(!t.check_tree());
        t.root->keys()[0] = old;
        CHECK(t.check_tree());
    }
}

TEST_CASE("bts btree: non-trivial values and keys")
{
    REQUIRE(tracked::instances() == 0);
    {
        auto values = std::vector<std::pair<std::string, tracked>>{};
        for (auto i = 0; i < 200; ++i)
            values.push_back({pad(i * 7), tracked{i}});

        auto t = build_packed<tracked_tree>(values);
        REQUIRE(t.check_tree());
        CHECK(t.size == 200u);

        for (auto i = 0; i < 200; ++i) {
            auto p = find_ptr(t, pad(i * 7));
            REQUIRE(p != nullptr);
            CHECK(p->second.value == i);
        }
        CHECK(find_ptr(t, pad(3)) == nullptr);
        CHECK(find_ptr(t, std::string{"999999"}) == nullptr);

        CHECK(tracked::instances() == 400); // the vector and the tree
    }
    CHECK(tracked::instances() == 0);
}

TEST_CASE("bts btree: heterogeneous lookup with a transparent comparator")
{
    auto values = std::vector<std::pair<std::string, int>>{};
    for (auto i = 0; i < 100; ++i)
        values.push_back({pad(i * 3), i});

    auto t = build_packed<map_tree>(values);
    REQUIRE(t.check_tree());

    auto p = find_ptr(t, "000030");
    REQUIRE(p != nullptr);
    CHECK(p->second == 10);
    CHECK(find_ptr(t, "000031") == nullptr);
}

TEST_CASE("bts btree: alternative memory policy")
{
    auto keys = spread_keys(500, 2);
    auto t    = build_packed<unsafe_tree>(keys);
    REQUIRE(t.check_tree());
    for (auto k : keys)
        REQUIRE(find_ptr(t, k) != nullptr);
    CHECK(find_ptr(t, 1) == nullptr);
}

TEST_CASE("bts btree: chunked traversal")
{
    for (auto n : {0, 1, 5, 100, 1000}) {
        auto keys = spread_keys(n, 3);
        auto t    = build_packed<small_tree>(keys);
        auto out  = std::vector<int>{};
        t.for_each_chunk([&](const int* fst, const int* lst) {
            out.insert(out.end(), fst, lst);
        });
        CHECK(out == keys);

        auto count = 0;
        auto more  = t.for_each_chunk_p([&](const int* fst, const int* lst) {
            count += static_cast<int>(lst - fst);
            return count < n / 2;
        });
        CHECK(!more); // the predicate stops the traversal half way
        CHECK(count <= n);
        if (n > 0)
            CHECK(count >= n / 2);
    }
}

TEST_CASE("bts btree: debug stats")
{
    auto t     = build_packed<small_tree>(spread_keys(100, 1));
    auto stats = t.get_debug_stats();
    CHECK(stats.value_count == 100u);
    CHECK(stats.leaf_count >= 25u);
    CHECK(stats.leaf_count <= 50u);
    CHECK(stats.inner_count > 0u);
    CHECK(stats.bits == 2u);
    CHECK(stats.bits_leaf == 2u);
}
