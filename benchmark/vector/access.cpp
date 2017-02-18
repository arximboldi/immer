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

#include <nonius/nonius_single.h++>

#include "util.hpp"

#include <immer/algorithm.hpp>
#include <immer/array.hpp>
#include <immer/flex_vector.hpp>
#include <immer/vector.hpp>

#if IMMER_BENCHMARK_EXPERIMENTAL
#include <immer/experimental/dvektor.hpp>
#endif

#if IMMER_BENCHMARK_STEADY
#define QUARK_ASSERT_ON 0
#include <steady_vector.h>
#endif

#if IMMER_BENCHMARK_LIBRRB
extern "C" {
#define restrict __restrict__
#include <rrb.h>
#undef restrict
}
#include <immer/heap/gc_heap.hpp>
#endif

#if IMMER_BENCHMARK_BOOST_COROUTINE
#include <boost/coroutine2/all.hpp>
#endif

#include <vector>
#include <list>
#include <numeric>

NONIUS_PARAM(N, std::size_t{1000})

auto make_generator(std::size_t runs)
{
    assert(runs > 0);
    auto engine = std::default_random_engine{42};
    auto dist = std::uniform_int_distribution<std::size_t>{0, runs-1};
    auto r = std::vector<std::size_t>(runs);
    std::generate_n(r.begin(), runs, std::bind(dist, engine));
    return r;
}

NONIUS_BENCHMARK("std::vector", [] (nonius::parameters params)
{
    auto n = params.get<N>();

    auto v = std::vector<unsigned>(n);
    std::iota(v.begin(), v.end(), 0u);

    return [=] {
        auto volatile x = std::accumulate(v.begin(), v.end(), 0u);
        return x;
    };
})

NONIUS_BENCHMARK("std::vector/idx", [] (nonius::parameters params)
{
    auto n = params.get<N>();

    auto v = std::vector<unsigned>(n);
    std::iota(v.begin(), v.end(), 0u);

    return [=] {
        auto r = 0u;
        for (auto i = 0u; i < n; ++i)
            r += v[i];
        volatile auto rr = r;
        return rr;
    };
})

NONIUS_BENCHMARK("std::vector/random", [] (nonius::parameters params)
{
    auto n = params.get<N>();

    auto g = make_generator(n);
    auto v = std::vector<unsigned>(n);
    std::iota(v.begin(), v.end(), 0u);

    return [=] {
        auto r = 0u;
        for (auto i = 0u; i < n; ++i)
            r += v[g[i]];
        volatile auto rr = r;
        return rr;
    };
})

NONIUS_BENCHMARK("std::list", [] (nonius::parameters params)
{
    auto n = params.get<N>();

    auto v = std::list<unsigned>{};
    for (auto i = 0u; i < n; ++i)
        v.push_back(i);

    return [=] {
        auto volatile x = std::accumulate(v.begin(), v.end(), 0u);
        return x;
    };
})

#if IMMER_BENCHMARK_LIBRRB
NONIUS_BENCHMARK("librrb", [] (nonius::parameters params)
{
    auto n = params.get<N>();

    auto v = rrb_create();
    for (auto i = 0u; i < n; ++i)
        v = rrb_push(v, reinterpret_cast<void*>(i));

    return [=] {
        auto r = 0u;
        for (auto i = 0u; i < n; ++i)
            r += reinterpret_cast<unsigned long>(rrb_nth(v, i));
        volatile auto rr = r;
        return rr;
    };
})

NONIUS_BENCHMARK("librrb/F", [] (nonius::parameters params)
{
    auto n = params.get<N>();

    auto v = rrb_create();
    for (auto i = 0u; i < n; ++i) {
        auto f = rrb_push(rrb_create(),
                          reinterpret_cast<void*>(i));
        v = rrb_concat(f, v);
    }

    return [=] {
        auto r = 0u;
        for (auto i = 0u; i < n; ++i)
            r += reinterpret_cast<unsigned long>(rrb_nth(v, i));
        volatile auto rr = r;
        return rr;
    };
})

NONIUS_BENCHMARK("librrb/random", [] (nonius::parameters params)
{
    auto n = params.get<N>();

    auto v = rrb_create();
    for (auto i = 0u; i < n; ++i)
        v = rrb_push(v, reinterpret_cast<void*>(i));
    auto g = make_generator(n);

    return [=] {
        auto r = 0u;
        for (auto i = 0u; i < n; ++i)
            r += reinterpret_cast<unsigned long>(rrb_nth(v, g[i]));
        volatile auto rr = r;
        return rr;
    };
})

NONIUS_BENCHMARK("librrb/F/random", [] (nonius::parameters params)
{
    auto n = params.get<N>();

    auto v = rrb_create();
    for (auto i = 0u; i < n; ++i) {
        auto f = rrb_push(rrb_create(),
                          reinterpret_cast<void*>(i));
        v = rrb_concat(f, v);
    }

    auto g = make_generator(n);

    return [=] {
        volatile auto r = 0u;
        for (auto i = 0u; i < n; ++i)
            r += reinterpret_cast<unsigned long>(rrb_nth(v, g[i]));
        return r;
    };
})
#endif

template <typename Vektor, typename PushFn=push_back_fn>
auto generic_iter()
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
auto generic_coro()
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
auto generic_idx()
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
auto generic_reduce()
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
auto generic_random()
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

using def_memory = immer::default_memory_policy;

NONIUS_BENCHMARK("flex/5B",     generic_iter<immer::flex_vector<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("flex/F/5B",   generic_iter<immer::flex_vector<unsigned,def_memory,5>,push_front_fn>())
NONIUS_BENCHMARK("vector/4B",   generic_iter<immer::vector<unsigned,def_memory,4>>())
NONIUS_BENCHMARK("vector/5B",   generic_iter<immer::vector<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("vector/6B",   generic_iter<immer::vector<unsigned,def_memory,6>>())

#if IMMER_BENCHMARK_EXPERIMENTAL
NONIUS_BENCHMARK("dvektor/4B",  generic_iter<immer::dvektor<unsigned,def_memory,4>>())
NONIUS_BENCHMARK("dvektor/5B",  generic_iter<immer::dvektor<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("dvektor/6B",  generic_iter<immer::dvektor<unsigned,def_memory,6>>())
#endif

#if IMMER_BENCHMARK_STEADY
NONIUS_BENCHMARK("steady/idx",      generic_idx<steady::vector<unsigned>>())
#endif

NONIUS_BENCHMARK("flex/5B/idx",     generic_idx<immer::flex_vector<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("flex/F/5B/idx",   generic_idx<immer::flex_vector<unsigned,def_memory,5>,push_front_fn>())
NONIUS_BENCHMARK("vector/4B/idx",   generic_idx<immer::vector<unsigned,def_memory,4>>())
NONIUS_BENCHMARK("vector/5B/idx",   generic_idx<immer::vector<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("vector/6B/idx",   generic_idx<immer::vector<unsigned,def_memory,6>>())
#if IMMER_BENCHMARK_EXPERIMENTAL
NONIUS_BENCHMARK("dvektor/4B/idx",  generic_idx<immer::dvektor<unsigned,def_memory,4>>())
NONIUS_BENCHMARK("dvektor/5B/idx",  generic_idx<immer::dvektor<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("dvektor/6B/idx",  generic_idx<immer::dvektor<unsigned,def_memory,6>>())
#endif

NONIUS_BENCHMARK("flex/5B/reduce",   generic_reduce<immer::flex_vector<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("flex/F/5B/reduce", generic_reduce<immer::flex_vector<unsigned,def_memory,5>,push_front_fn>())
NONIUS_BENCHMARK("vector/4B/reduce", generic_reduce<immer::vector<unsigned,def_memory,4>>())
NONIUS_BENCHMARK("vector/5B/reduce", generic_reduce<immer::vector<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("vector/6B/reduce", generic_reduce<immer::vector<unsigned,def_memory,6>>())

#if IMMER_BENCHMARK_STEADY
NONIUS_BENCHMARK("steady/random",      generic_random<steady::vector<unsigned>>())
#endif

NONIUS_BENCHMARK("flex/5B/random",     generic_random<immer::flex_vector<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("flex/F/5B/random",   generic_random<immer::flex_vector<unsigned,def_memory,5>, push_front_fn>())
NONIUS_BENCHMARK("vector/4B/random",   generic_random<immer::vector<unsigned,def_memory,4>>())
NONIUS_BENCHMARK("vector/5B/random",   generic_random<immer::vector<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("vector/6B/random",   generic_random<immer::vector<unsigned,def_memory,6>>())
#if IMMER_BENCHMARK_EXPERIMENTAL
NONIUS_BENCHMARK("dvektor/4B/random",  generic_random<immer::dvektor<unsigned,def_memory,4>>())
NONIUS_BENCHMARK("dvektor/5B/random",  generic_random<immer::dvektor<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("dvektor/6B/random",  generic_random<immer::dvektor<unsigned,def_memory,6>>())
#endif

#if IMMER_BENCHMARK_BOOST_COROUTINE
NONIUS_BENCHMARK("vector/5B/coro", generic_coro<immer::vector<unsigned,def_memory,5>>())
#endif
