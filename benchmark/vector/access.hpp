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

#include "benchmark/vector/common.hpp"

#include <immer/algorithm.hpp>

#if IMMER_BENCHMARK_BOOST_COROUTINE
#include <boost/coroutine2/all.hpp>
#endif

namespace {

template <typename Vektor>
auto benchmark_access_iter_std()
{
    return [] (nonius::parameters params)
    {
        auto n = params.get<N>();
        auto v = Vektor{};
        for (auto i = 0u; i < n; ++i)
            v.push_back(i);
        return [=] {
            auto volatile x = std::accumulate(v.begin(), v.end(), 0u);
            return x;
        };
    };
}

template <typename Vektor>
auto benchmark_access_idx_std()
{
    return [] (nonius::parameters params)
    {
        auto n = params.get<N>();
        auto v = Vektor{};
        for (auto i = 0u; i < n; ++i)
            v.push_back(i);
        return [=] {
            auto r = 0u;
            for (auto i = 0u; i < n; ++i)
                r += v[i];
            volatile auto rr = r;
            return rr;
        };
    };
}

template <typename Vektor>
auto benchmark_access_random_std()
{
    return [] (nonius::parameters params)
    {
        auto n = params.get<N>();
        auto v = Vektor{};
        auto g = make_generator(n);
        for (auto i = 0u; i < n; ++i)
            v.push_back(i);
        return [=] {
            auto r = 0u;
            for (auto i = 0u; i < n; ++i)
                r += v[g[i]];
            volatile auto rr = r;
            return rr;
        };
    };
}

template <typename Vektor, typename PushFn=push_back_fn>
auto benchmark_access_iter()
{
    return [] (nonius::parameters params)
    {
        auto n = params.get<N>();

        auto v = Vektor{};
        for (auto i = 0u; i < n; ++i)
            v = PushFn{}(std::move(v), i);

        return [=] {
            auto volatile x = std::accumulate(v.begin(), v.end(), 0u);
            return x;
        };
    };
}

#if IMMER_BENCHMARK_BOOST_COROUTINE
template <typename Vektor, typename PushFn=push_back_fn>
auto benchmark_access_coro()
{
    return [] (nonius::parameters params)
    {
        using coro_t = typename boost::coroutines2::coroutine<int>;

        auto n = params.get<N>();

        auto v = Vektor{};
        for (auto i = 0u; i < n; ++i)
            v = PushFn{}(std::move(v), i);

        return [=] {
            auto c = coro_t::pull_type { [&](auto& sink) {
                v.for_each_chunk([&](auto f, auto l) {
                    for (; f != l; ++f)
                        sink(*f);
                });
            }};
            auto volatile x = std::accumulate(begin(c), end(c), 0u);
            return x;
        };
    };
}
#endif

template <typename Vektor,
          typename PushFn=push_back_fn>
auto benchmark_access_idx()
{
    return [] (nonius::parameters params)
    {
        auto n = params.get<N>();

        auto v = Vektor{};
        for (auto i = 0u; i < n; ++i)
            v = PushFn{}(std::move(v), i);

        return [=] {
            auto r = 0u;
            for (auto i = 0u; i < n; ++i)
                r += v[i];
            volatile auto rr = r;
            return rr;
        };
    };
};

template <typename Vektor,
          typename PushFn=push_back_fn>
auto benchmark_access_reduce()
{
    return [] (nonius::parameters params)
    {
        auto n = params.get<N>();

        auto v = Vektor{};
        for (auto i = 0u; i < n; ++i)
            v = PushFn{}(std::move(v), i);

        return [=] {
            auto volatile x = immer::accumulate(v, 0u);
            return x;
        };
    };
}

template <typename Vektor,
          typename PushFn=push_back_fn>
auto benchmark_access_random()
{
    return [] (nonius::parameters params)
    {
        auto n = params.get<N>();

        auto v = Vektor{};
        for (auto i = 0u; i < n; ++i)
            v = PushFn{}(std::move(v), i);
        auto g = make_generator(n);

        return [=] {
            auto r = 0u;
            for (auto i = 0u; i < n; ++i)
                r += v[g[i]];
            volatile auto rr = r;
            return rr;
        };
    };
};

template <typename Fn>
auto benchmark_access_librrb(Fn maker)
{
    return
        [=] (nonius::parameters params) {
            auto n = params.get<N>();
            auto v = maker(n);
            return
                [=] {
                    auto r = 0u;
                    for (auto i = 0u; i < n; ++i)
                        r += reinterpret_cast<unsigned long>(rrb_nth(v, i));
                    volatile auto rr = r;
                    return rr;
                };
        };
}

template <typename Fn>
auto benchmark_access_random_librrb(Fn maker)
{
    return
        [=] (nonius::parameters params) {
            auto n = params.get<N>();
            auto v = maker(n);
            auto g = make_generator(n);
            return
                [=] {
                    auto r = 0u;
                    for (auto i = 0u; i < n; ++i)
                        r += reinterpret_cast<unsigned long>(rrb_nth(v, g[i]));
                    volatile auto rr = r;
                    return rr;
                };
        };
}

} // anonymous namespace
