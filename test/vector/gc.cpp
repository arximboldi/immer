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

#include <immer/vector.hpp>
#include <immer/heap/gc_heap.hpp>
#include <immer/refcount/no_refcount_policy.hpp>

using gc_memory = immer::memory_policy<
    immer::heap_policy<immer::gc_heap>,
    immer::no_refcount_policy,
    immer::gc_transience_policy,
    false>;

template <typename T>
using test_vector_t = immer::vector<T, gc_memory, 3u>;

#define VECTOR_T test_vector_t
#include "generic.ipp"
