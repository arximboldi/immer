//
// immer - immutable data structures for C++
// Copyright (C) 2016, 2017 Juan Pedro Bolivar Puente
//
// This file is part of immer.
//
// immer is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// immer is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with immer.  If not, see <http://www.gnu.org/licenses/>.
//

#pragma once

#include <utility>
#include <cstddef>
#include <limits>

#include "benchmark/config.hpp"

#if IMMER_BENCHMARK_LIBRRB
extern "C" {
#define restrict __restrict__
#include <rrb.h>
#undef restrict
}
#include <immer/heap/gc_heap.hpp>
#endif

namespace immer {
template <typename T> class array;
} // namespace immer

namespace {

auto make_generator(std::size_t runs)
{
    assert(runs > 0);
    auto engine = std::default_random_engine{42};
    auto dist = std::uniform_int_distribution<std::size_t>{0, runs-1};
    auto r = std::vector<std::size_t>(runs);
    std::generate_n(r.begin(), runs, std::bind(dist, engine));
    return r;
}

struct push_back_fn
{
    template <typename T, typename U>
    auto operator() (T&& v, U&& x)
    { return std::forward<T>(v).push_back(std::forward<U>(x)); }
};

struct push_front_fn
{
    template <typename T, typename U>
    auto operator() (T&& v, U&& x)
    { return std::forward<T>(v).push_front(std::forward<U>(x)); }
};

struct set_fn
{
    template <typename T, typename I, typename U>
    decltype(auto) operator() (T&& v, I i, U&& x)
    { return std::forward<T>(v).set(i, std::forward<U>(x)); }
};

struct store_fn
{
    template <typename T, typename I, typename U>
    decltype(auto) operator() (T&& v, I i, U&& x)
    { return std::forward<T>(v).store(i, std::forward<U>(x)); }
};

template <typename T>
struct get_limit : std::integral_constant<
    std::size_t, std::numeric_limits<std::size_t>::max()> {};

template <typename T>
struct get_limit<immer::array<T>> : std::integral_constant<
    std::size_t, 10000> {};

auto make_librrb_vector(std::size_t n)
{
    auto v = rrb_create();
    for (auto i = 0u; i < n; ++i) {
        v = rrb_push(v, reinterpret_cast<void*>(i));
    }
    return v;
}

auto make_librrb_vector_f(std::size_t n)
{
    auto v = rrb_create();
    for (auto i = 0u; i < n; ++i) {
        auto f = rrb_push(rrb_create(),
                          reinterpret_cast<void*>(i));
        v = rrb_concat(f, v);
    }
    return v;
}

} // anonymous namespace
