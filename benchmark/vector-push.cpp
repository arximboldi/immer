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
#include <numeric>

#if IMMER_BENCHMARK_LIBRRB
extern "C" {
#define restrict __restrict__
#include <rrb.h>
#undef restrict
}
#endif

NONIUS_PARAM(N, std::size_t{1000})

NONIUS_BENCHMARK("std::vector", [] (nonius::parameters params)
{
    auto n = params.get<N>();

    return [=] {
        auto v = std::vector<unsigned>{};
        for (auto i = 0u; i < n; ++i)
            v.push_back(i);
        return v;
    };
})

NONIUS_BENCHMARK("std::list", [] (nonius::parameters params)
{
    auto n = params.get<N>();

    return [=] {
        auto v = std::list<unsigned>{};
        for (auto i = 0u; i < n; ++i)
            v.push_back(i);
        return v;
    };
})

#if IMMER_BENCHMARK_LIBRRB
NONIUS_BENCHMARK("librrb", [] (nonius::parameters params)
{
    auto n = params.get<N>();

    return [=] {
        auto v = rrb_create();
        for (auto i = 0u; i < n; ++i)
            v = rrb_push(v, reinterpret_cast<void*>(i));
        return v;
    };
})
#endif

template <typename Vektor>
auto generic()
{
    return [] (nonius::parameters params)
    {
        auto n = params.get<N>();
        if (n > get_limit<Vektor>{})
            nonius::skip();

        return [=] {
            auto v = Vektor{};
            for (auto i = 0u; i < n; ++i)
                v = v.push_back(i);
            return v;
        };
    };
};

using gc_memory     = immer::memory_policy<immer::heap_policy<immer::gc_heap>, immer::no_refcount_policy>;
using basic_memory  = immer::memory_policy<immer::heap_policy<immer::malloc_heap>, immer::refcount_policy>;
using unsafe_memory = immer::memory_policy<immer::default_heap_policy, immer::unsafe_refcount_policy>;

NONIUS_BENCHMARK("flex/5B",    generic<immer::flex_vector<unsigned,5>>())
NONIUS_BENCHMARK("flex_s/GC",  generic<immer::flex_vector<std::size_t,5,gc_memory>>())

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

NONIUS_BENCHMARK("array",    generic<immer::array<unsigned>>())
