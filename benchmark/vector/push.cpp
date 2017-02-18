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
#include <immer/vector_transient.hpp>
#include <immer/vector.hpp>

#if IMMER_BENCHMARK_EXPERIMENTAL
#include <immer/experimental/dvektor.hpp>
#endif

#include <immer/heap/gc_heap.hpp>
#include <immer/refcount/no_refcount_policy.hpp>
#include <immer/refcount/unsafe_refcount_policy.hpp>

#include <vector>
#include <list>
#include <numeric>

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

#if IMMER_BENCHMARK_LIBRRB
NONIUS_BENCHMARK("librrb", [] (nonius::chronometer meter)
{
    auto n = meter.param<N>();

    measure(meter, [&] {
        auto v = rrb_create();
        for (auto i = 0u; i < n; ++i)
            v = rrb_push(v, reinterpret_cast<void*>(i));
        return v;
    });
})

NONIUS_BENCHMARK("t/librrb", [] (nonius::chronometer meter)
{
    auto n = meter.param<N>();

    measure(meter, [&] {
        auto v = rrb_to_transient(rrb_create());
        for (auto i = 0u; i < n; ++i)
            v = transient_rrb_push(v, reinterpret_cast<void*>(i));
        return v;
    });
})
#endif

template <typename Vektor>
auto generic_mut()
{
    return [] (nonius::chronometer meter)
    {
        auto n = meter.param<N>();
        if (n > get_limit<Vektor>{})
            nonius::skip();

        measure(meter, [&] {
            auto v = Vektor{};
            for (auto i = 0u; i < n; ++i)
                v.push_back(i);
            return v;
        });
    };
};

template <typename Vektor>
auto generic_move()
{
    return [] (nonius::chronometer meter)
    {
        auto n = meter.param<N>();
        if (n > get_limit<Vektor>{})
            nonius::skip();

        measure(meter, [&] {
            auto v = Vektor{};
            for (auto i = 0u; i < n; ++i)
                v = std::move(v).push_back(i);
            return v;
        });
    };
};

template <typename Vektor>
auto generic()
{
    return [] (nonius::chronometer meter)
    {
        auto n = meter.param<N>();
        if (n > get_limit<Vektor>{})
            nonius::skip();

        measure(meter, [&] {
            auto v = Vektor{};
            for (auto i = 0u; i < n; ++i)
                v = v.push_back(i);
            return v;
        });
    };
};

NONIUS_BENCHMARK("std::vector", generic_mut<std::vector<unsigned>>())
NONIUS_BENCHMARK("std::list",   generic_mut<std::list<unsigned>>())

using def_memory    = immer::default_memory_policy;
using gc_memory     = immer::memory_policy<immer::heap_policy<immer::gc_heap>, immer::no_refcount_policy>;
using basic_memory  = immer::memory_policy<immer::heap_policy<immer::malloc_heap>, immer::refcount_policy>;
using unsafe_memory = immer::memory_policy<immer::default_heap_policy, immer::unsafe_refcount_policy>;

NONIUS_BENCHMARK("m/vector/5B", generic_move<immer::vector<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("m/vector/GC", generic_move<immer::vector<unsigned,gc_memory,5>>())
NONIUS_BENCHMARK("m/vector/NO", generic_move<immer::vector<unsigned,basic_memory,5>>())

NONIUS_BENCHMARK("t/vector/5B", generic_mut<immer::vector_transient<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("t/vector/GC", generic_mut<immer::vector_transient<unsigned,gc_memory,5>>())
NONIUS_BENCHMARK("t/vector/NO", generic_mut<immer::vector_transient<unsigned,basic_memory,5>>())
NONIUS_BENCHMARK("t/vector/UN", generic_mut<immer::vector_transient<unsigned,unsafe_memory,5>>())

NONIUS_BENCHMARK("flex/5B",    generic<immer::flex_vector<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("flex_s/GC",  generic<immer::flex_vector<std::size_t,gc_memory,5>>())

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
NONIUS_BENCHMARK("steady",     generic<steady::vector<unsigned>>())
#endif
