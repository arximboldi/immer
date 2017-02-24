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

#include "benchmark/vector/push.hpp"
#include <immer/flex_vector.hpp>
#include <immer/flex_vector_transient.hpp>

NONIUS_BENCHMARK("ours/basic",   benchmark_push<immer::flex_vector<unsigned,basic_memory>>())
NONIUS_BENCHMARK("ours/safe",    benchmark_push<immer::flex_vector<unsigned,def_memory>>())
NONIUS_BENCHMARK("ours/unsafe",  benchmark_push<immer::flex_vector<unsigned,unsafe_memory>>())
NONIUS_BENCHMARK("ours/gc",      benchmark_push<immer::flex_vector<unsigned,gc_memory>>())
NONIUS_BENCHMARK("librrb",       benchmark_push_librrb)

NONIUS_BENCHMARK("transient ours/basic",   benchmark_push_mut<immer::flex_vector<unsigned,basic_memory>>())
NONIUS_BENCHMARK("transient ours/safe",    benchmark_push_mut<immer::flex_vector<unsigned,def_memory>>())
NONIUS_BENCHMARK("transient ours/unsafe",  benchmark_push_mut<immer::flex_vector<unsigned,unsafe_memory>>())
NONIUS_BENCHMARK("transient ours/gc",      benchmark_push_mut<immer::flex_vector<unsigned,gc_memory>>())
NONIUS_BENCHMARK("transient librrb",       benchmark_push_mut_librrb)
NONIUS_BENCHMARK("transient std::vector",  benchmark_push_mut_std<std::vector<unsigned>>())
NONIUS_BENCHMARK("transient std::list",    benchmark_push_mut_std<std::list<unsigned>>())
