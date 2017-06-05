//
// immer - immutable data structures for C++
// Copyright (C) 2016, 2017 Juan Pedro Bolivar Puente
//
// This file is part of immer.
//
// immer is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// immer is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with immer.  If not, see <http://www.gnu.org/licenses/>.
//

#include "benchmark/vector/concat.hpp"
#include <immer/flex_vector.hpp>
#include <immer/flex_vector_transient.hpp>
#include <chunkedseq.hpp>

NONIUS_BENCHMARK("ours/basic",   benchmark_concat_incr<immer::flex_vector<unsigned,basic_memory>>())
NONIUS_BENCHMARK("ours/safe",    benchmark_concat_incr<immer::flex_vector<unsigned,def_memory>>())
NONIUS_BENCHMARK("ours/unsafe",  benchmark_concat_incr<immer::flex_vector<unsigned,unsafe_memory>>())
NONIUS_BENCHMARK("ours/gc",      benchmark_concat_incr<immer::flex_vector<unsigned,gc_memory>>())
NONIUS_BENCHMARK("librrb",       benchmark_concat_incr_librrb(make_librrb_vector))

NONIUS_BENCHMARK("transient ours/gc", benchmark_concat_incr_mut<immer::flex_vector<unsigned,gc_memory>>())
NONIUS_BENCHMARK("transient chunkedseq32", benchmark_concat_incr_chunkedseq<pasl::data::chunkedseq::bootstrapped::deque<unsigned, 32>>())
