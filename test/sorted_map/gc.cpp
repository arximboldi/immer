//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include <immer/heap/gc_heap.hpp>
#include <immer/refcount/no_refcount_policy.hpp>
#include <immer/sorted_map.hpp>

using gc_memory = immer::memory_policy<immer::heap_policy<immer::gc_heap>,
                                       immer::no_refcount_policy,
                                       immer::default_lock_policy,
                                       immer::gc_transience_policy,
                                       false>;

template <typename K, typename T, typename Compare = std::less<K>>
using test_sorted_map_t = immer::sorted_map<K, T, Compare, gc_memory, 3u, 3u>;

#define SORTED_MAP_T test_sorted_map_t
#define IMMER_IS_GC_TEST 1
#include "generic.ipp"
