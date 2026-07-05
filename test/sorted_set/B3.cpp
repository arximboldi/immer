//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include <immer/sorted_set.hpp>

template <typename T, typename Compare = std::less<T>>
using test_sorted_set_t =
    immer::sorted_set<T, Compare, immer::default_memory_policy, 3u, 3u>;

#define SORTED_SET_T test_sorted_set_t
#include "generic.ipp"
