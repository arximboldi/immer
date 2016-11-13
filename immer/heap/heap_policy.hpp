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
 * todo
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

struct free_list_heap_policy
{
    template <std::size_t... Sizes>
    struct apply
    {
        static constexpr auto node_size = std::max({Sizes...});

        using type = with_free_list_node<
            thread_local_free_list_heap<
                node_size,
                malloc_heap>>;
    };
};

} // namespace immer
