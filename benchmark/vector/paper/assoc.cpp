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

#include "benchmark/vector/assoc.hpp"
#include <immer/flex_vector.hpp>
#include <immer/flex_vector_transient.hpp>

NONIUS_BENCHMARK("ours/basic",   benchmark_assoc<immer::flex_vector<unsigned,basic_memory>>())
NONIUS_BENCHMARK("ours/safe",    benchmark_assoc<immer::flex_vector<unsigned,def_memory>>())
NONIUS_BENCHMARK("ours/unsafe",  benchmark_assoc<immer::flex_vector<unsigned,unsafe_memory>>())
NONIUS_BENCHMARK("ours/gc",      benchmark_assoc<immer::flex_vector<unsigned,gc_memory>>())
NONIUS_BENCHMARK("librrb",       benchmark_assoc_librrb(make_librrb_vector))

NONIUS_BENCHMARK("relaxed ours/basic",   benchmark_assoc<immer::flex_vector<unsigned,basic_memory>,push_front_fn>())
NONIUS_BENCHMARK("relaxed ours/safe",    benchmark_assoc<immer::flex_vector<unsigned,def_memory>,push_front_fn>())
NONIUS_BENCHMARK("relaxed ours/unsafe",  benchmark_assoc<immer::flex_vector<unsigned,unsafe_memory>,push_front_fn>())
NONIUS_BENCHMARK("relaxed ours/gc",      benchmark_assoc<immer::flex_vector<unsigned,gc_memory>,push_front_fn>())
NONIUS_BENCHMARK("relaxed librrb",       benchmark_assoc_librrb(make_librrb_vector_f))

NONIUS_BENCHMARK("transient ours/basic",   benchmark_assoc_mut<immer::flex_vector<unsigned,basic_memory>>())
NONIUS_BENCHMARK("transient ours/safe",    benchmark_assoc_mut<immer::flex_vector<unsigned,def_memory>>())
NONIUS_BENCHMARK("transient ours/unsafe",  benchmark_assoc_mut<immer::flex_vector<unsigned,unsafe_memory>>())
NONIUS_BENCHMARK("transient ours/gc",      benchmark_assoc_mut<immer::flex_vector<unsigned,gc_memory>>())
NONIUS_BENCHMARK("transient librrb",       benchmark_assoc_mut_librrb(make_librrb_vector))

NONIUS_BENCHMARK("transient relaxed ours/basic",   benchmark_assoc_mut<immer::flex_vector<unsigned,basic_memory>,push_back_fn>())
NONIUS_BENCHMARK("transient relaxed ours/safe",    benchmark_assoc_mut<immer::flex_vector<unsigned,def_memory>,push_back_fn>())
NONIUS_BENCHMARK("transient relaxed ours/unsafe",  benchmark_assoc_mut<immer::flex_vector<unsigned,unsafe_memory>,push_back_fn>())
NONIUS_BENCHMARK("transient relaxed ours/gc",      benchmark_assoc_mut<immer::flex_vector<unsigned,gc_memory>,push_back_fn>())
NONIUS_BENCHMARK("transient relaxed librrb",       benchmark_assoc_mut_librrb(make_librrb_vector_f))

NONIUS_BENCHMARK("transient std::vector",  benchmark_assoc_std<std::vector<unsigned>>())
