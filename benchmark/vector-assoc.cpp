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

#include <immer/array.hpp>
#include <immer/flex_vector.hpp>
#include <immer/flex_vector_transient.hpp>
#include <immer/vector.hpp>
#include <immer/vector_transient.hpp>

#if IMMER_BENCHMARK_EXPERIMENTAL
#include <immer/experimental/dvektor.hpp>
#endif

#include <immer/heap/gc_heap.hpp>
#include <immer/refcount/no_refcount_policy.hpp>
#include <immer/refcount/unsafe_refcount_policy.hpp>

#include <vector>
#include <list>

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

    measure(meter, [&] {
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

    measure(meter, [&] {
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

    measure(meter, [&] {
        auto r = v;
        for (auto i = 0u; i < n; ++i)
            r = rrb_update(r, g[i], reinterpret_cast<void*>(i));
        return r;
    });
})

NONIUS_BENCHMARK("t/librrb", [] (nonius::chronometer meter)
{
    auto n = meter.param<N>();

    auto v = rrb_to_transient(rrb_create());
    for (auto i = 0u; i < n; ++i)
        v = transient_rrb_push(v, reinterpret_cast<void*>(i));
    auto vv = transient_to_rrb(v);

    measure(meter, [&] {
        auto r = rrb_to_transient(vv);
        for (auto i = 0u; i < n; ++i)
            r = transient_rrb_update(r, i, reinterpret_cast<void*>(n - i));
        return r;
    });
})

NONIUS_BENCHMARK("t/librrb/F", [] (nonius::chronometer meter)
{
    auto n = meter.param<N>();

    auto v = rrb_create();
    for (auto i = 0u; i < n; ++i) {
        auto f = rrb_push(rrb_create(),
                          reinterpret_cast<void*>(i));
        v = rrb_concat(f, v);
    }

    measure(meter, [&] {
        auto r = rrb_to_transient(v);
        for (auto i = 0u; i < n; ++i)
            r = transient_rrb_update(r, i, reinterpret_cast<void*>(n - i));
        return r;
    });
})

NONIUS_BENCHMARK("t/librrb/random", [] (nonius::chronometer meter)
{
    auto n = meter.param<N>();

    auto g = make_generator(n);
    auto v = rrb_to_transient(rrb_create());
    for (auto i = 0u; i < n; ++i)
        v = transient_rrb_push(v, reinterpret_cast<void*>(i));
    auto vv = transient_to_rrb(v);

    measure(meter, [&] {
        auto r = rrb_to_transient(vv);
        for (auto i = 0u; i < n; ++i)
            r = transient_rrb_update(r, g[i], reinterpret_cast<void*>(n - i));
        return r;
    });
})
#endif

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

template <typename Vektor,
          typename PushFn=push_back_fn,
          typename SetFn=set_fn>
auto generic()
{
    return [] (nonius::chronometer meter)
    {
        auto n = meter.param<N>();
        if (n > get_limit<Vektor>{})
            nonius::skip();

        auto v = Vektor{};
        for (auto i = 0u; i < n; ++i)
            v = PushFn{}(std::move(v), i);

        measure(meter, [&] {
            auto r = v;
            for (auto i = 0u; i < n; ++i)
                r = SetFn{}(r, i, n - i);
            return r;
        });
    };
};

template <typename Vektor,
          typename PushFn=push_back_fn>
auto generic_move()
{
    return [] (nonius::chronometer meter)
    {
        auto n = meter.param<N>();
        if (n > get_limit<Vektor>{})
            nonius::skip();

        auto v = Vektor{};
        for (auto i = 0u; i < n; ++i)
            v = PushFn{}(std::move(v), i);

        measure(meter, [&] {
            auto r = v;
            for (auto i = 0u; i < n; ++i)
                r = std::move(r).set(i, n - i);
            return r;
        });
    };
};

template <typename Vektor,
          typename SetFn=set_fn>
auto generic_random()
{
    return [] (nonius::chronometer meter)
    {
        auto n = meter.param<N>();
        if (n > get_limit<Vektor>{})
            nonius::skip();

        auto g = make_generator(n);
        auto v = Vektor{};
        for (auto i = 0u; i < n; ++i)
            v = v.push_back(i);

        measure(meter, [&] {
            auto r = v;
            for (auto i = 0u; i < n; ++i)
                r = SetFn{}(r, g[i], i);
            return r;
        });
    };
};

template <typename Vektor,
          typename PushFn=push_back_fn>
auto generic_mut()
{
    return [] (nonius::chronometer meter)
    {
        auto n = meter.param<N>();
        if (n > get_limit<Vektor>{})
            nonius::skip();

        auto v = Vektor{};
        for (auto i = 0u; i < n; ++i)
            v = PushFn{}(std::move(v), i);

        measure(meter, [&] {
            auto r = v.transient();
            for (auto i = 0u; i < n; ++i)
                r.set(i, n - i);
            return r;
        });
    };
};

template <typename Vektor,
          typename PushFn=push_back_fn>
auto generic_mut_random()
{
    return [] (nonius::chronometer meter)
    {
        auto n = meter.param<N>();
        if (n > get_limit<Vektor>{})
            nonius::skip();

        auto g = make_generator(n);
        auto v = Vektor{};
        for (auto i = 0u; i < n; ++i)
            v = PushFn{}(std::move(v), i);

        measure(meter, [&] {
            auto r = v.transient();
            for (auto i = 0u; i < n; ++i)
                r.set(g[i], i);
            return r;
        });
    };
};

using def_memory    = immer::default_memory_policy;
using gc_memory     = immer::memory_policy<immer::heap_policy<immer::gc_heap>, immer::no_refcount_policy>;
using gcf_memory    = immer::memory_policy<immer::heap_policy<immer::gc_heap>, immer::no_refcount_policy, immer::gc_transience_policy, false>;
using basic_memory  = immer::memory_policy<immer::heap_policy<immer::malloc_heap>, immer::refcount_policy>;
using unsafe_memory = immer::memory_policy<immer::default_heap_policy, immer::unsafe_refcount_policy>;

#if IMMER_BENCHMARK_STEADY
NONIUS_BENCHMARK("steady",     generic<steady::vector<unsigned>, push_back_fn, store_fn>())
#endif

NONIUS_BENCHMARK("flex/5B",    generic<immer::flex_vector<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("flex/F/5B",  generic<immer::flex_vector<unsigned,def_memory,5>,push_front_fn>())

NONIUS_BENCHMARK("flex/GC",    generic<immer::flex_vector<unsigned,gc_memory,5>>())
NONIUS_BENCHMARK("flex/F/GC",  generic<immer::flex_vector<unsigned,gc_memory,5>,push_front_fn>())
NONIUS_BENCHMARK("flex/F/GCF", generic<immer::flex_vector<unsigned,gcf_memory,5>,push_front_fn>())

NONIUS_BENCHMARK("flex_s/GC",  generic<immer::flex_vector<std::size_t,gc_memory,5>>())
NONIUS_BENCHMARK("flex_s/F/GC",generic<immer::flex_vector<std::size_t,gc_memory,5>,push_front_fn>())
NONIUS_BENCHMARK("flex_s/F/GCF",generic<immer::flex_vector<std::size_t,gcf_memory,5>,push_front_fn>())

NONIUS_BENCHMARK("vector/4B",  generic<immer::vector<unsigned,def_memory,4>>())
NONIUS_BENCHMARK("vector/5B",  generic<immer::vector<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("vector/6B",  generic<immer::vector<unsigned,def_memory,6>>())

NONIUS_BENCHMARK("vector/GC",  generic<immer::vector<unsigned,gc_memory,5>>())
NONIUS_BENCHMARK("vector/NO",  generic<immer::vector<unsigned,basic_memory,5>>())
NONIUS_BENCHMARK("vector/UN",  generic<immer::vector<unsigned,unsafe_memory,5>>())

#if IMMER_BENCHMARK_EXPERIMENTAL
NONIUS_BENCHMARK("dvektor/4B", generic<immer::dvektor<unsigned,def_memory,4>>())
NONIUS_BENCHMARK("dvektor/5B", generic<immer::dvektor<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("dvektor/6B", generic<immer::dvektor<unsigned,def_memory,6>>())

NONIUS_BENCHMARK("dvektor/GC", generic<immer::dvektor<unsigned,gc_memory,5>>())
NONIUS_BENCHMARK("dvektor/NO", generic<immer::dvektor<unsigned,basic_memory,5>>())
NONIUS_BENCHMARK("dvektor/UN", generic<immer::dvektor<unsigned,unsafe_memory,5>>())
#endif

NONIUS_BENCHMARK("array",      generic<immer::array<unsigned>>())

#if IMMER_BENCHMARK_STEADY
NONIUS_BENCHMARK("steady/random",      generic_random<steady::vector<unsigned>, store_fn>())
#endif

NONIUS_BENCHMARK("flex/5B/random",     generic_random<immer::flex_vector<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("vector/4B/random",   generic_random<immer::vector<unsigned,def_memory,4>>())
NONIUS_BENCHMARK("vector/5B/random",   generic_random<immer::vector<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("vector/6B/random",   generic_random<immer::vector<unsigned,def_memory,6>>())
#if IMMER_BENCHMARK_EXPERIMENTAL
NONIUS_BENCHMARK("dvektor/4B/random",  generic_random<immer::dvektor<unsigned,def_memory,4>>())
NONIUS_BENCHMARK("dvektor/5B/random",  generic_random<immer::dvektor<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("dvektor/6B/random",  generic_random<immer::dvektor<unsigned,def_memory,6>>())
#endif
NONIUS_BENCHMARK("array/random",       generic_random<immer::array<unsigned>>())

NONIUS_BENCHMARK("t/vector/5B",  generic_mut<immer::vector<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("t/vector/GC",  generic_mut<immer::vector<unsigned,gc_memory,5>>())
NONIUS_BENCHMARK("t/vector/NO",  generic_mut<immer::vector<unsigned,basic_memory,5>>())
NONIUS_BENCHMARK("t/vector/UN",  generic_mut<immer::vector<unsigned,unsafe_memory,5>>())
NONIUS_BENCHMARK("t/flex/F/5B",  generic_mut<immer::flex_vector<unsigned,def_memory,5>,push_front_fn>())

NONIUS_BENCHMARK("m/vector/5B",  generic_move<immer::vector<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("m/vector/GC",  generic_move<immer::vector<unsigned,gc_memory,5>>())
NONIUS_BENCHMARK("m/vector/NO",  generic_move<immer::vector<unsigned,basic_memory,5>>())
NONIUS_BENCHMARK("m/vector/UN",  generic_move<immer::vector<unsigned,unsafe_memory,5>>())
NONIUS_BENCHMARK("m/flex/F/5B",  generic_move<immer::flex_vector<unsigned,def_memory,5>,push_front_fn>())

NONIUS_BENCHMARK("t/vector/5B/random",  generic_mut_random<immer::vector<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("t/vector/GC/random",  generic_mut_random<immer::vector<unsigned,gc_memory,5>>())
NONIUS_BENCHMARK("t/vector/NO/random",  generic_mut_random<immer::vector<unsigned,basic_memory,5>>())
NONIUS_BENCHMARK("t/vector/UN/random",  generic_mut_random<immer::vector<unsigned,unsafe_memory,5>>())
NONIUS_BENCHMARK("t/flex/F/5B/random",  generic_mut_random<immer::flex_vector<unsigned,def_memory,5>,push_front_fn>())
