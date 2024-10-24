//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include <immer/flex_vector.hpp>
#include <immer/memory_policy.hpp>
#include <immer/vector.hpp>

using unsafe_memory =
    immer::memory_policy<immer::unsafe_free_list_heap_policy<immer::cpp_heap>,
                         immer::no_refcount_policy,
                         immer::no_lock_policy>;

template <typename T>
using test_flex_vector_t = immer::flex_vector<T, unsafe_memory, 3u, 6u>;

template <typename T>
using test_vector_t = immer::vector<T, unsafe_memory, 3u, 6u>;

#define FLEX_VECTOR_T test_flex_vector_t
#define VECTOR_T test_vector_t
#include "generic.ipp"
