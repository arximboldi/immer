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

#ifndef MEMORY_T
#error "define the MEMORY_T"
#endif

NONIUS_BENCHMARK("flex/3", benchmark_push<immer::flex_vector<std::size_t,MEMORY_T,3>>())
NONIUS_BENCHMARK("flex/4", benchmark_push<immer::flex_vector<std::size_t,MEMORY_T,4>>())
NONIUS_BENCHMARK("flex/5", benchmark_push<immer::flex_vector<std::size_t,MEMORY_T,5>>())
NONIUS_BENCHMARK("flex/6", benchmark_push<immer::flex_vector<std::size_t,MEMORY_T,6>>())
NONIUS_BENCHMARK("flex/7", benchmark_push<immer::flex_vector<std::size_t,MEMORY_T,7>>())
