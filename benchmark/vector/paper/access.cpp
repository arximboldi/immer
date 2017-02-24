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

#include "benchmark/vector/access.hpp"
#include <immer/flex_vector.hpp>

NONIUS_BENCHMARK("reduce owrs", benchmark_access_reduce<immer::flex_vector<unsigned,def_memory>>())
NONIUS_BENCHMARK("iter orws", benchmark_access_iter<immer::flex_vector<unsigned,def_memory>>())
NONIUS_BENCHMARK("idx owrs", benchmark_access_idx<immer::flex_vector<unsigned,def_memory>>())
NONIUS_BENCHMARK("idx librrb", benchmark_access_librrb(make_librrb_vector))

NONIUS_BENCHMARK("relaxed reduce owrs", benchmark_access_reduce<immer::flex_vector<unsigned,def_memory>,push_front_fn>())
NONIUS_BENCHMARK("relaxed iter orws", benchmark_access_iter<immer::flex_vector<unsigned,def_memory>,push_front_fn>())
NONIUS_BENCHMARK("relaxed idx owrs", benchmark_access_idx<immer::flex_vector<unsigned,def_memory>,push_front_fn>())
NONIUS_BENCHMARK("relaxed idx librrb", benchmark_access_librrb(make_librrb_vector_f))

NONIUS_BENCHMARK("std::vector", benchmark_access_iter_std<std::vector<unsigned>>())
NONIUS_BENCHMARK("std::list", benchmark_access_iter_std<std::list<unsigned>>())
