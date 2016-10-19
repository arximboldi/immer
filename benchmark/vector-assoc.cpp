//
// immer - immutable data structures for C++
// Copyright (C) 2016 Juan Pedro Bolivar Puente
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

#include <immer/vector.hpp>
#include <immer/array.hpp>
#include <immer/flex_vector.hpp>

#if IMMER_BENCHMARK_EXPERIMENTAL
#include <immer/experimental/dvektor.hpp>
#endif

#include <immer/heap/gc_heap.hpp>
#include <immer/refcount/no_refcount_policy.hpp>
#include <immer/refcount/unsafe_refcount_policy.hpp>

#include <vector>
#include <list>

#if IMMER_BENCHMARK_LIBRRB
extern "C" {
#define restrict __restrict__
#include <rrb.h>
#undef restrict
}
#endif

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

NONIUS_BENCHMARK("std::vector", [] (nonius::chronometer meter)
{
    auto n = meter.param<N>();

    auto v = std::vector<unsigned>(n);
    std::iota(v.begin(), v.end(), 0u);

    auto all = std::vector<std::vector<unsigned>>(meter.runs(), v);

    meter.measure([&] (int iter) {
        auto& r = all[iter];
        for (auto i = 0u; i < n; ++i)
            r[i] = n - i;
        return r;
    });
})

NONIUS_BENCHMARK("std::vector/random", [] (nonius::chronometer meter)
{
    auto n = meter.param<N>();

    auto g = make_generator(n);
    auto v = std::vector<unsigned>(n);
    std::iota(v.begin(), v.end(), 0u);

    auto all = std::vector<std::vector<unsigned>>(meter.runs(), v);

    meter.measure([&] (int iter) {
        auto& r = all[iter];
        for (auto i = 0u; i < n; ++i)
            r[g[i]] = i;
        return r;
    });
})

#if IMMER_BENCHMARK_LIBRRB
NONIUS_BENCHMARK("librrb", [] (nonius::chronometer meter)
{
    auto n = meter.param<N>();

    auto v = rrb_create();
    for (auto i = 0u; i < n; ++i)
        v = rrb_push(v, reinterpret_cast<void*>(i));

    meter.measure([&] {
        auto r = v;
        for (auto i = 0u; i < n; ++i)
            r = rrb_update(r, i, reinterpret_cast<void*>(n - i));
        return r;
    });
})

NONIUS_BENCHMARK("librrb/F", [] (nonius::chronometer meter)
{
    auto n = meter.param<N>();

    auto v = rrb_create();
    for (auto i = 0u; i < n; ++i) {
        auto f = rrb_push(rrb_create(),
                          reinterpret_cast<void*>(i));
        v = rrb_concat(f, v);
    }

    meter.measure([&] {
        auto r = v;
        for (auto i = 0u; i < n; ++i)
            r = rrb_update(r, i, reinterpret_cast<void*>(n - i));
        return r;
    });
})

NONIUS_BENCHMARK("librrb/random", [] (nonius::chronometer meter)
{
    auto n = meter.param<N>();

    auto g = make_generator(n);
    auto v = rrb_create();
    for (auto i = 0u; i < n; ++i)
        v = rrb_push(v, reinterpret_cast<void*>(i));

    meter.measure([&] {
        auto r = v;
        for (auto i = 0u; i < n; ++i)
            r = rrb_update(r, g[i], reinterpret_cast<void*>(i));
        return r;
    });
})
#endif

template <typename Vektor,
          typename PushFn=push_back_fn>
auto generic()
{
    return [] (nonius::chronometer meter)
    {
        auto n = meter.param<N>();
        if (n > get_limit<Vektor>{}) n = 1;

        auto v = Vektor{};
        for (auto i = 0u; i < n; ++i)
            v = PushFn{}(std::move(v), i);

        meter.measure([&] {
            auto r = v;
            for (auto i = 0u; i < n; ++i)
                r = v.assoc(i, n - i);
            return r;
        });
    };
};

using gc_memory     = immer::memory_policy<immer::heap_policy<immer::gc_heap>, immer::no_refcount_policy>;
using gcf_memory    = immer::memory_policy<immer::heap_policy<immer::gc_heap>, immer::no_refcount_policy, false>;
using basic_memory  = immer::memory_policy<immer::heap_policy<immer::malloc_heap>, immer::refcount_policy>;
using unsafe_memory = immer::memory_policy<immer::default_heap_policy, immer::unsafe_refcount_policy>;

NONIUS_BENCHMARK("flex/5B",    generic<immer::flex_vector<unsigned,5>>())
NONIUS_BENCHMARK("flex/F/5B",  generic<immer::flex_vector<unsigned,5>,push_front_fn>())
NONIUS_BENCHMARK("flex/GC",    generic<immer::flex_vector<unsigned,5,gc_memory>>())
NONIUS_BENCHMARK("flex_s/GC",  generic<immer::flex_vector<std::size_t,5,gc_memory>>())
NONIUS_BENCHMARK("flex/F/GC",  generic<immer::flex_vector<unsigned,5,gc_memory>,push_front_fn>())
NONIUS_BENCHMARK("flex/F/GCF", generic<immer::flex_vector<unsigned,5,gcf_memory>,push_front_fn>())
NONIUS_BENCHMARK("flex_s/F/GC",generic<immer::flex_vector<std::size_t,5,gc_memory>,push_front_fn>())

NONIUS_BENCHMARK("vector/4B",  generic<immer::vector<unsigned,4>>())
NONIUS_BENCHMARK("vector/5B",  generic<immer::vector<unsigned,5>>())
NONIUS_BENCHMARK("vector/6B",  generic<immer::vector<unsigned,6>>())

NONIUS_BENCHMARK("vector/GC",  generic<immer::vector<unsigned,5,gc_memory>>())
NONIUS_BENCHMARK("vector/NO",  generic<immer::vector<unsigned,5,basic_memory>>())
NONIUS_BENCHMARK("vector/UN",  generic<immer::vector<unsigned,5,unsafe_memory>>())

#if IMMER_BENCHMARK_EXPERIMENTAL
NONIUS_BENCHMARK("dvektor/4B", generic<immer::dvektor<unsigned,4>>())
NONIUS_BENCHMARK("dvektor/5B", generic<immer::dvektor<unsigned,5>>())
NONIUS_BENCHMARK("dvektor/6B", generic<immer::dvektor<unsigned,6>>())

NONIUS_BENCHMARK("dvektor/GC", generic<immer::dvektor<unsigned,5,gc_memory>>())
NONIUS_BENCHMARK("dvektor/NO", generic<immer::dvektor<unsigned,5,basic_memory>>())
NONIUS_BENCHMARK("dvektor/UN", generic<immer::dvektor<unsigned,5,unsafe_memory>>())
#endif

NONIUS_BENCHMARK("array",      generic<immer::array<unsigned>>())

template <typename Vektor>
auto generic_random()
{
    return [] (nonius::parameters params)
    {
        auto n = params.get<N>();
        if (n > get_limit<Vektor>{}) n = 1;

        auto g = make_generator(n);
        auto v = Vektor{};
        for (auto i = 0u; i < n; ++i)
            v = v.push_back(i);

        return [=] {
            auto r = v;
            for (auto i = 0u; i < n; ++i)
                r = v.assoc(g[i], i);
            return r;
        };
    };
};

NONIUS_BENCHMARK("flex/5B/random",     generic_random<immer::flex_vector<unsigned,5>>())
NONIUS_BENCHMARK("vector/4B/random",   generic_random<immer::vector<unsigned,4>>())
NONIUS_BENCHMARK("vector/5B/random",   generic_random<immer::vector<unsigned,5>>())
NONIUS_BENCHMARK("vector/6B/random",   generic_random<immer::vector<unsigned,6>>())
#if IMMER_BENCHMARK_EXPERIMENTAL
NONIUS_BENCHMARK("dvektor/4B/random",  generic_random<immer::dvektor<unsigned,4>>())
NONIUS_BENCHMARK("dvektor/5B/random",  generic_random<immer::dvektor<unsigned,5>>())
NONIUS_BENCHMARK("dvektor/6B/random",  generic_random<immer::dvektor<unsigned,6>>())
#endif
NONIUS_BENCHMARK("array/random",       generic_random<immer::array<unsigned>>())
