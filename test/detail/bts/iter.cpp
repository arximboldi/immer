//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include <immer/detail/bts/btree_iterator.hpp>
#include <immer/memory_policy.hpp>

#include "test/detail/bts/util.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <functional>
#include <iterator>
#include <map>
#include <random>
#include <utility>
#include <vector>

using namespace immer::detail::bts;
using namespace btstest;

namespace {

using int_small = btree<int,
                        identity_fn,
                        std::less<int>,
                        immer::default_memory_policy,
                        2u,
                        2u>;

using int_set = btree<int,
                      identity_fn,
                      std::less<int>,
                      immer::default_memory_policy,
                      5u,
                      5u>;

using pair_small = btree<std::pair<int, int>,
                         first_fn,
                         std::less<int>,
                         immer::default_memory_policy,
                         2u,
                         2u>;

template <typename Tree>
struct iter_for;

template <typename T, typename KF, typename C, typename MP, bits_t B, bits_t BL>
struct iter_for<btree<T, KF, C, MP, B, BL>>
{
    using type = btree_iterator<T, KF, C, MP, B, BL>;
};

template <typename Tree>
using iter_for_t = typename iter_for<Tree>::type;

static_assert(
    std::is_same<std::iterator_traits<iter_for_t<int_small>>::iterator_category,
                 std::bidirectional_iterator_tag>::value,
    "");
static_assert(
    std::is_same<std::iterator_traits<iter_for_t<int_small>>::value_type,
                 int>::value,
    "");

template <typename Tree>
auto begin_of(const Tree& t)
{
    return iter_for_t<Tree>{t};
}

template <typename Tree>
auto end_of(const Tree& t)
{
    return iter_for_t<Tree>{t, typename iter_for_t<Tree>::end_t{}};
}

template <typename Tree>
auto collect(const Tree& t)
{
    auto out = std::vector<typename Tree::value_t>{};
    for (auto it = begin_of(t), e = end_of(t); it != e; ++it)
        out.push_back(*it);
    return out;
}

template <typename Tree>
auto collect_rev(const Tree& t)
{
    auto out = std::vector<typename Tree::value_t>{};
    auto b   = begin_of(t);
    auto it  = end_of(t);
    if (t.size > 0u) {
        do {
            --it;
            out.push_back(*it);
        } while (it != b);
    }
    return out;
}

template <typename Tree>
Tree tree_of(const std::vector<int>& keys)
{
    auto t = Tree::empty();
    for (auto k : keys)
        t = t.add(k);
    return t;
}

} // namespace

TEST_CASE("bts iter: empty tree")
{
    auto t = int_small::empty();
    CHECK(begin_of(t) == end_of(t));
    CHECK(collect(t).empty());
    CHECK(collect_rev(t).empty());
}

TEST_CASE("bts iter: forward iteration visits keys in order")
{
    for (auto n : {1, 2, 3, 5, 17, 64, 100, 257}) {
        auto keys = spread_keys(n, 3);
        auto shuf = keys;
        std::shuffle(shuf.begin(), shuf.end(), std::default_random_engine{7});
        auto t = tree_of<int_small>(shuf);
        REQUIRE(t.check_tree());
        CHECK(collect(t) == keys);
        CHECK(std::distance(begin_of(t), end_of(t)) ==
              static_cast<std::ptrdiff_t>(n));
    }
    for (auto n : {1000, 20000}) {
        auto keys = spread_keys(n, 2);
        auto t    = build_packed<int_set>(keys);
        REQUIRE(t.check_tree());
        CHECK(collect(t) == keys);
    }
}

TEST_CASE("bts iter: backward iteration visits keys in reverse order")
{
    for (auto n : {1, 2, 5, 100, 257}) {
        auto keys = spread_keys(n, 3);
        auto t    = tree_of<int_small>(keys);
        auto rev  = keys;
        std::reverse(rev.begin(), rev.end());
        CHECK(collect_rev(t) == rev);
    }
    auto keys = spread_keys(5000, 2);
    auto t    = build_packed<int_set>(keys);
    auto rev  = keys;
    std::reverse(rev.begin(), rev.end());
    CHECK(collect_rev(t) == rev);
}

TEST_CASE("bts iter: increment and decrement round trips")
{
    auto keys = spread_keys(100, 1);
    auto t    = tree_of<int_small>(keys);
    auto it   = begin_of(t);
    for (auto i = 0; i < 100; ++i, ++it) {
        REQUIRE(*it == i);
        auto j = it;
        ++j;
        if (j != end_of(t)) {
            --j;
            REQUIRE(j == it);
        }
    }
    REQUIRE(it == end_of(t));
    --it;
    CHECK(*it == 99);
    ++it;
    CHECK(it == end_of(t));
}

TEST_CASE("bts iter: post increment and decrement")
{
    auto t  = tree_of<int_small>(spread_keys(10, 1));
    auto it = begin_of(t);
    CHECK(*(it++) == 0);
    CHECK(*it == 1);
    CHECK(*(it--) == 1);
    CHECK(*it == 0);
}

TEST_CASE("bts iter: works with std::reverse_iterator")
{
    auto keys = spread_keys(300, 2);
    auto t    = tree_of<int_small>(keys);
    auto out  = std::vector<int>{};
    auto rb   = std::reverse_iterator<iter_for_t<int_small>>{end_of(t)};
    auto re   = std::reverse_iterator<iter_for_t<int_small>>{begin_of(t)};
    for (; rb != re; ++rb)
        out.push_back(*rb);
    auto rev = keys;
    std::reverse(rev.begin(), rev.end());
    CHECK(out == rev);
}

TEST_CASE("bts iter: arrow operator on pair values")
{
    auto t = pair_small::empty();
    for (auto k : spread_keys(50, 2))
        t = t.add({k, k + 1});
    auto it = begin_of(t);
    CHECK(it->first == 0);
    CHECK(it->second == 1);
    ++it;
    CHECK(it->first == 2);
}

TEST_CASE("bts iter: lower and upper bound")
{
    auto check_bounds = [](auto& t, const std::vector<int>& keys) {
        using it_t = iter_for_t<std::decay_t<decltype(t)>>;
        auto end   = end_of(t);
        auto lo    = keys.empty() ? 0 : keys.front() - 2;
        auto hi    = keys.empty() ? 0 : keys.back() + 2;
        for (auto probe = lo; probe <= hi; ++probe) {
            auto olb = std::lower_bound(keys.begin(), keys.end(), probe);
            auto oub = std::upper_bound(keys.begin(), keys.end(), probe);
            auto tlb = it_t{t, probe, typename it_t::lower_bound_t{}};
            auto tub = it_t{t, probe, typename it_t::upper_bound_t{}};
            if (olb == keys.end())
                REQUIRE(tlb == end);
            else
                REQUIRE(*tlb == *olb);
            if (oub == keys.end())
                REQUIRE(tub == end);
            else
                REQUIRE(*tub == *oub);
        }
    };

    auto keys = spread_keys(200, 3);
    auto t    = tree_of<int_small>(keys);
    check_bounds(t, keys);

    auto big_keys = spread_keys(5000, 2, 10);
    auto big      = build_packed<int_set>(big_keys);
    check_bounds(big, big_keys);

    auto e     = int_small::empty();
    using it_t = iter_for_t<int_small>;
    CHECK(it_t{e, 42, it_t::lower_bound_t{}} == end_of(e));
    CHECK(it_t{e, 42, it_t::upper_bound_t{}} == end_of(e));
}

TEST_CASE("bts iter: iterating shared versions")
{
    auto keys = spread_keys(500, 2);
    auto t    = tree_of<int_small>(keys);
    auto t2   = t.add(999).sub(0);
    CHECK(collect(t) == keys);
    auto keys2 = std::vector<int>{keys.begin() + 1, keys.end()};
    keys2.push_back(999);
    CHECK(collect(t2) == keys2);
}
