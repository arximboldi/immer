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

#include <immer/heap/free_list_heap.hpp>
#include <immer/heap/malloc_heap.hpp>
#include <immer/heap/thread_local_free_list_heap.hpp>
#include <immer/config.hpp>

#include <cstdlib>
#include <algorithm>

namespace immer {

/*!
 * Heap policy that unconditionally uses its `Heap` argument.
 */
template <typename Heap>
struct heap_policy
{
    template <std::size_t...>
    struct apply
    {
        using type = Heap;
    };
};

template <typename Deriv, typename HeapPolicy>
struct enable_heap_policy
{
    static void* operator new (std::size_t size)
    {
        using heap_type = typename HeapPolicy
            ::template apply<sizeof(Deriv)>::type;

        return heap_type::allocate(size);
    }

    static void operator delete (void* data)
    {
        using heap_type = typename HeapPolicy
            ::template apply<sizeof(Deriv)>::type;

        heap_type::deallocate(data);
    }
};

/*!
 * Heap policy that creates returns heap with a free list of objects
 * of `max_size = max(Sizes...)` on top an underlying `Heap`.  Note
 * these two properties of the resulting heap:
 *
 * - Allocating an object that is bigger than `max_size` may trigger
 *   *undefined behavior*.
 *
 * - Allocating an object of with size less that `max_size` still
 *   returns an object of `max_size`.
 *
 * Basically, this heap will always return objects of `max_size`.
 * When an object is freed, it does not directly invoke `std::free`,
 * but it keeps the object in a global linked list instead.  When a
 * new object is requested, it does not need to call `std::malloc` but
 * it can directly pop and return the other object from the global
 * list -- a much faster operation.
 *
 * This actually creates a hierarchy with two free lists:
 *
 * - A `thread_local` free list is used first.  It does not need any
 *   kind of synchronization and is very fast.  When the thread
 *   finishes, its contents are returned to the next free list.
 *
 * - A global free list using lock-free access via atomics.
 *
 * @tparam Heap Heap to be used when the free list is empty.
 *
 * @rst
 *
 * .. tip:: For many applications that use immutable data structures
 *    significantly, this is actually the best heap policy, and it
 *    might become the default in the future.
 *
 *    Note that most our data structures internally use trees with the
 *    same big branching factors.  This means that all *vectors*,
 *    *maps*, etc. can just allocate elements from the same free-list
 *    optimized heap.  Not only does this lower the allocation time,
 *    but also makes up for more efficient *cache utilization*.  When
 *    a new node is needed, there are high chances the allocator will
 *    return a node that was just accessed.  When batches of immutable
 *    updates are made, this can make a significant difference.
 *
 * .. admonition:: Todo
 *
 *    It might be a bad idea to use this allocator when immutable data
 *    structures are seldom used, specially if, when used, very big
 *    structures requiring a lot of memory are created.  If no
 *    immutable data is created later, this memory is wasted. We
 *    should add *bound* the maximum amount of memory that is allowed
 *    to be parked in the free list.
 *
 * @endrst
 */
template <typename Heap>
struct free_list_heap_policy
{
    template <std::size_t... Sizes>
    struct apply
    {
        static constexpr auto max_size = std::max({Sizes...});

        using type = with_free_list_node<
            thread_local_free_list_heap<
                max_size,
                free_list_heap<max_size,
                               Heap>>>;
    };
};

} // namespace immer
