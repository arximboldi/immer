//
// immer - immutable data structures for C++
// Copyright (C) 2016 Juan Pedro Bolivar Puente
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

#pragma once

#include <immer/heap/heap_policy.hpp>
#include <immer/refcount/refcount_policy.hpp>
#include <type_traits>

namespace immer {

/*!
 * A memory policy.
 */
template <typename HeapPolicy,
          typename RefcountPolicy,
          bool PreferFewerBiggerObjects = !std::is_same<
              HeapPolicy,
              free_list_heap_policy>()>
struct memory_policy
{
    using heap     = HeapPolicy;
    using refcount = RefcountPolicy;

    static constexpr bool prefer_fewer_bigger_objects =
        PreferFewerBiggerObjects;
};

#if IMMER_FREE_LIST
using default_heap_policy = free_list_heap_policy;
#else
using default_heap_policy = heap_policy<malloc_heap>;
#endif

using default_refcount_policy = refcount_policy;

using default_memory_policy = memory_policy<
    default_heap_policy,
    default_refcount_policy>;

} // namespace immer
