//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include <immer/detail/bts/node.hpp>
#include <immer/memory_policy.hpp>

#include "test/detail/bts/util.hpp"

#include <catch2/catch_test_macros.hpp>

#include <new>
#include <stdexcept>
#include <string>

using btstest::throwing_key;
using btstest::tracked;

namespace {

template <typename T, typename K>
using node_t =
    immer::detail::bts::node<T, K, immer::default_memory_policy, 5u, 5u>;

using leaf_node_t  = node_t<tracked, int>;
using inner_node_t = node_t<tracked, std::string>;

template <typename NodeT>
NodeT* make_tracked_leaf(int first, int count)
{
    auto p =
        NodeT::make_leaf_n(static_cast<immer::detail::bts::count_t>(count));
    auto v = p->values();
    for (auto i = 0; i < count; ++i)
        new (v + i) tracked{first + i};
    return p;
}

} // namespace

TEST_CASE("bts node: make and delete leaf")
{
    REQUIRE(tracked::instances() == 0);
    {
        auto p = make_tracked_leaf<leaf_node_t>(0, 3);
        CHECK(p->count() == 3u);
        CHECK(p->values()[0].value == 0);
        CHECK(p->values()[2].value == 2);
        CHECK(leaf_node_t::refs(p).unique());
        leaf_node_t::delete_leaf(p);
    }
    CHECK(tracked::instances() == 0);
}

TEST_CASE("bts node: copy leaf")
{
    REQUIRE(tracked::instances() == 0);
    {
        auto p = make_tracked_leaf<leaf_node_t>(10, 5);
        auto q = leaf_node_t::copy_leaf(p);
        CHECK(tracked::instances() == 10);
        CHECK(q->count() == 5u);
        CHECK(q->values()[0].value == 10);
        CHECK(q->values()[4].value == 14);
        leaf_node_t::delete_leaf(p);
        CHECK(tracked::instances() == 5);
        leaf_node_t::delete_leaf(q);
    }
    CHECK(tracked::instances() == 0);
}

#ifndef IMMER_NO_EXCEPTIONS
TEST_CASE("bts node: copy leaf cleans up when an element copy throws")
{
    REQUIRE(tracked::instances() == 0);
    {
        auto p = make_tracked_leaf<leaf_node_t>(0, 4);
        CHECK(tracked::instances() == 4);
        tracked::copies_until_throw() = 2;
        CHECK_THROWS_AS(leaf_node_t::copy_leaf(p), std::runtime_error);
        tracked::copies_until_throw() = -1;
        CHECK(tracked::instances() == 4);
        leaf_node_t::delete_leaf(p);
    }
    CHECK(tracked::instances() == 0);
}
#endif

TEST_CASE("bts node: inc and dec")
{
    auto p = make_tracked_leaf<leaf_node_t>(0, 1);
    CHECK(leaf_node_t::refs(p).unique());
    p->inc();
    CHECK(!leaf_node_t::refs(p).unique());
    CHECK(!p->dec());
    CHECK(leaf_node_t::refs(p).unique());
    CHECK(p->dec());
    leaf_node_t::delete_leaf(p);
    CHECK(tracked::instances() == 0);
}

TEST_CASE("bts node: inner nodes with non-trivial keys")
{
    REQUIRE(tracked::instances() == 0);
    {
        auto l1 = make_tracked_leaf<inner_node_t>(0, 2);
        auto l2 = make_tracked_leaf<inner_node_t>(2, 2);

        auto p           = inner_node_t::make_inner_n(2);
        p->children()[0] = l1;
        p->children()[1] = l2;
        p->sizes()[0]    = 2;
        p->sizes()[1]    = 4;
        new (p->keys()) std::string{"a rather long separator key 000002"};

        CHECK(p->count() == 2u);
        CHECK(inner_node_t::refs(l1).unique());

        auto q = inner_node_t::copy_inner(p);
        CHECK(q->count() == 2u);
        CHECK(q->children()[0] == l1);
        CHECK(q->keys()[0] == p->keys()[0]);
        CHECK(q->sizes()[1] == 4u);
        CHECK(!inner_node_t::refs(l1).unique());
        CHECK(!inner_node_t::refs(l2).unique());

        inner_node_t::delete_deep(p, 1u);
        CHECK(tracked::instances() == 4);
        CHECK(inner_node_t::refs(l1).unique());
        inner_node_t::delete_deep(q, 1u);
    }
    CHECK(tracked::instances() == 0);
}

#ifndef IMMER_NO_EXCEPTIONS
TEST_CASE("bts node: copy inner cleans up when a key copy throws")
{
    using node_type = node_t<int, throwing_key>;

    auto make_int_leaf = [](int first, int count) {
        auto p = node_type::make_leaf_n(
            static_cast<immer::detail::bts::count_t>(count));
        auto v = p->values();
        for (auto i = 0; i < count; ++i)
            new (v + i) int{first + i};
        return p;
    };

    auto l1 = make_int_leaf(0, 2);
    auto l2 = make_int_leaf(2, 2);
    auto l3 = make_int_leaf(4, 2);

    auto p           = node_type::make_inner_n(3);
    p->children()[0] = l1;
    p->children()[1] = l2;
    p->children()[2] = l3;
    p->sizes()[0]    = 2;
    p->sizes()[1]    = 4;
    p->sizes()[2]    = 6;
    new (p->keys()) throwing_key{"k2"};
    new (p->keys() + 1) throwing_key{"k4"};

    throwing_key::copies_until_throw() = 1;
    CHECK_THROWS_AS(node_type::copy_inner(p), std::runtime_error);
    throwing_key::copies_until_throw() = -1;

    CHECK(node_type::refs(l1).unique());
    CHECK(node_type::refs(l2).unique());
    CHECK(node_type::refs(l3).unique());

    node_type::delete_deep(p, 1u);
}
#endif
