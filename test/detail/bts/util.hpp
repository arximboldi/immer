//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <immer/config.hpp>
#include <immer/detail/bts/bits.hpp>

#include <cstdio>
#include <new>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace btstest {

struct tracked
{
    static int& instances()
    {
        static int v = 0;
        return v;
    }

    // negative means never throw
    static int& copies_until_throw()
    {
        static int v = -1;
        return v;
    }

    int value;

    tracked(int v = 0)
        : value{v}
    {
        ++instances();
    }

    tracked(const tracked& other)
        : value{other.value}
    {
#ifndef IMMER_NO_EXCEPTIONS
        if (copies_until_throw() == 0)
            throw std::runtime_error{"dada!"};
#endif
        if (copies_until_throw() > 0)
            --copies_until_throw();
        ++instances();
    }

    tracked(tracked&& other) noexcept
        : value{other.value}
    {
        ++instances();
    }

    tracked& operator=(const tracked&) = default;
    tracked& operator=(tracked&&)      = default;

    ~tracked() { --instances(); }
};

struct throwing_key
{
    // negative means never throw
    static int& copies_until_throw()
    {
        static int v = -1;
        return v;
    }

    std::string value;

    throwing_key(std::string s)
        : value{std::move(s)}
    {
    }

    throwing_key(const throwing_key& other)
        : value{other.value}
    {
#ifndef IMMER_NO_EXCEPTIONS
        if (copies_until_throw() == 0)
            throw std::runtime_error{"dada!"};
#endif
        if (copies_until_throw() > 0)
            --copies_until_throw();
    }

    throwing_key(throwing_key&&) noexcept        = default;
    throwing_key& operator=(const throwing_key&) = default;
    throwing_key& operator=(throwing_key&&)      = default;

    bool operator<(const throwing_key& other) const
    {
        return value < other.value;
    }
    bool operator==(const throwing_key& other) const
    {
        return value == other.value;
    }
};

struct identity_fn
{
    template <typename X>
    const X& operator()(const X& x) const
    {
        return x;
    }
};

struct first_fn
{
    template <typename P>
    const typename P::first_type& operator()(const P& p) const
    {
        return p.first;
    }
};

template <typename V>
struct project_ptr
{
    const V* operator()(const V& v) const { return &v; }
};

template <typename V>
struct null_ptr
{
    const V* operator()() const { return nullptr; }
};

template <typename Tree, typename Key>
const typename Tree::value_t* find_ptr(const Tree& t, const Key& k)
{
    using value_t = typename Tree::value_t;
    return t.template get<project_ptr<value_t>, null_ptr<value_t>>(k);
}

// Builds a packed tree out of values sorted by key.  This is a
// test-only stand-in for the `from_sorted` bulk constructor scheduled
// for milestone 2.
template <typename Tree, typename Vector>
Tree build_packed(const Vector& values)
{
    using node_t  = typename Tree::node_t;
    using key_t   = typename Tree::key_t;
    using value_t = typename Tree::value_t;
    using key_fn  = typename Tree::key_fn_t;

    using immer::detail::bts::count_t;
    using immer::detail::bts::local_size_t;

    constexpr auto leaf_cap =
        immer::detail::bts::branches<Tree::bits_leaf, std::size_t>;
    constexpr auto leaf_min =
        immer::detail::bts::min_branches<Tree::bits_leaf, std::size_t>;
    constexpr auto inner_cap =
        immer::detail::bts::branches<Tree::bits, std::size_t>;
    constexpr auto inner_min =
        immer::detail::bts::min_branches<Tree::bits, std::size_t>;

    if (values.empty())
        return Tree::empty();

    // all chunks of `cap` elements, except the last two get
    // redistributed so no chunk falls under `min`
    auto chunk_sizes = [](std::size_t total, std::size_t cap, std::size_t min) {
        auto out = std::vector<std::size_t>{};
        while (total > cap) {
            out.push_back(cap);
            total -= cap;
        }
        if (total < min && !out.empty()) {
            auto combined = out.back() + total;
            out.back()    = (combined + 1u) / 2u;
            total         = combined / 2u;
        }
        out.push_back(total);
        return out;
    };

    struct built
    {
        node_t* node;
        key_t first;
        local_size_t size;
    };

    auto level = std::vector<built>{};
    {
        auto it = values.begin();
        for (auto sz : chunk_sizes(values.size(), leaf_cap, leaf_min)) {
            auto p = node_t::make_leaf_n(static_cast<count_t>(sz));
            auto v = p->values();
            for (auto i = std::size_t{0}; i < sz; ++i, ++it)
                new (v + i) value_t{*it};
            level.push_back({p, key_fn{}(v[0]), static_cast<local_size_t>(sz)});
        }
    }

    auto depth = count_t{0};
    while (level.size() > 1u) {
        auto next = std::vector<built>{};
        auto it   = level.begin();
        for (auto sz : chunk_sizes(level.size(), inner_cap, inner_min)) {
            auto p     = node_t::make_inner_n(static_cast<count_t>(sz));
            auto total = local_size_t{0};
            for (auto i = std::size_t{0}; i < sz; ++i) {
                auto& child      = it[static_cast<std::ptrdiff_t>(i)];
                p->children()[i] = child.node;
                total += child.size;
                p->sizes()[i] = total;
                if (i > 0u)
                    new (p->keys() + (i - 1u)) key_t{child.first};
            }
            next.push_back({p, it->first, total});
            it += static_cast<std::ptrdiff_t>(sz);
        }
        level = std::move(next);
        ++depth;
    }

    return Tree{level[0].node, values.size(), depth};
}

inline std::vector<int> spread_keys(int count, int stride, int first = 0)
{
    auto keys = std::vector<int>{};
    for (auto i = 0; i < count; ++i)
        keys.push_back(first + i * stride);
    return keys;
}

inline std::string pad(int v)
{
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "%06d", v);
    return {buffer};
}

} // namespace btstest
