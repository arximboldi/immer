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

#include "benchmark/vector/push_front.hpp"

#include <immer/flex_vector.hpp>

#if IMMER_BENCHMARK_LIBRRB
extern "C" {
#define restrict __restrict__
#include <rrb.h>
#undef restrict
}
#endif

#if IMMER_BENCHMARK_LIBRRB
NONIUS_BENCHMARK("librrb", benchmark_push_front_librrb)
#endif
NONIUS_BENCHMARK("flex/4B", bechmark_push_front<immer::flex_vector<unsigned,def_memory,4>>())
NONIUS_BENCHMARK("flex/5B", bechmark_push_front<immer::flex_vector<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("flex/6B", bechmark_push_front<immer::flex_vector<unsigned,def_memory,6>>())
NONIUS_BENCHMARK("flex/GC", bechmark_push_front<immer::flex_vector<unsigned,gc_memory,5>>())
NONIUS_BENCHMARK("flex_s/GC", bechmark_push_front<immer::flex_vector<std::size_t,gc_memory,5>>())
NONIUS_BENCHMARK("flex/NO", bechmark_push_front<immer::flex_vector<unsigned,basic_memory,5>>())
NONIUS_BENCHMARK("flex/UN", bechmark_push_front<immer::flex_vector<unsigned,unsafe_memory,5>>())
