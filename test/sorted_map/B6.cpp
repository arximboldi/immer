//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include <immer/sorted_map.hpp>

template <typename K, typename T, typename Compare = std::less<K>>
using test_sorted_map_t =
    immer::sorted_map<K, T, Compare, immer::default_memory_policy, 6u, 6u>;

#define SORTED_MAP_T test_sorted_map_t
#include "generic.ipp"
