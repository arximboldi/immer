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

#include "insert.hpp"

#ifndef GENERATOR_T
#error "you must define a GENERATOR_T"
#endif

using generator__ = GENERATOR_T;
using t__ = typename decltype(generator__{}(0))::value_type;

NONIUS_BENCHMARK("std::set", benchmark_insert_mut_std<generator__, std::set<t__>>())
NONIUS_BENCHMARK("std::unordered_set", benchmark_insert_mut_std<generator__, std::unordered_set<t__>>())
NONIUS_BENCHMARK("boost::flat_set", benchmark_insert_mut_std<generator__, boost::container::flat_set<t__>>())
NONIUS_BENCHMARK("hamt::hash_trie", benchmark_insert_mut_std<generator__, hamt::hash_trie<t__>>())

NONIUS_BENCHMARK("immer::set/5B", benchmark_insert<generator__, immer::set<t__, std::hash<t__>,std::equal_to<t__>,def_memory,5>>())
NONIUS_BENCHMARK("immer::set/4B", benchmark_insert<generator__, immer::set<t__, std::hash<t__>,std::equal_to<t__>,def_memory,4>>())
#ifndef DISABLE_GC_BENCHMARKS
NONIUS_BENCHMARK("immer::set/GC", benchmark_insert<generator__, immer::set<t__, std::hash<t__>,std::equal_to<t__>,gc_memory,5>>())
#endif
NONIUS_BENCHMARK("immer::set/UN", benchmark_insert<generator__, immer::set<t__, std::hash<t__>,std::equal_to<t__>,unsafe_memory,5>>())
