//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include <immer/box.hpp>

#include <immer/array.hpp>
#include <immer/array_transient.hpp>
#include <immer/vector.hpp>
#include <immer/vector_transient.hpp>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("issue-33")
{
    using Element = immer::box<std::string>;
    auto vect     = immer::vector<Element>{};

    // this one works fine
    for (auto i = 0; i < 100; ++i) {
        vect = vect.push_back(Element("x"));
    }

    // this one doesn't compile
    auto t = vect.transient();
    for (auto i = 0; i < 100; ++i) {
        t.push_back(Element("x"));
    }
    vect = t.persistent();

    CHECK(*vect[0] == "x");
    CHECK(*vect[99] == "x");
}

namespace {
struct node_t
{
    using array_t = immer::array<immer::box<node_t>>;
    node_t()      = default;
    node_t(immer::box<int>) {}
    node_t(immer::box<double>) {}
    node_t(array_t) {}
};
} // namespace

TEST_CASE("pr-316")
{
    auto x = node_t::array_t{}.transient().persistent();
    CHECK(x.size() == 0);
}

namespace {
struct step_t
{};
using history_t = immer::vector<immer::box<step_t>>;
using summary_t = immer::vector<history_t>;
} // namespace

TEST_CASE("lager tree_debugger issue")
{
    auto step    = step_t{};
    auto hist    = history_t{}.push_back(step);
    auto summary = summary_t{}.push_back(hist);
    CHECK(true);
}
