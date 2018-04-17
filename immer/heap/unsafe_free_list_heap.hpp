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

#pragma once

#include <immer/config.hpp>
#include <immer/heap/free_list_node.hpp>
#include <cassert>

namespace immer {
namespace detail {

template <typename Heap>
struct unsafe_free_list_storage
{
    struct head_t
    {
        free_list_node* data;
        std::size_t count;
    };

    static head_t& head()
    {
        static head_t head_ {nullptr, 0};
        return head_;
    }
};

template <template<class>class Storage,
          std::size_t Size,
          std::size_t Limit,
          typename Base>
class unsafe_free_list_heap_impl : Base
{
    using storage = Storage<unsafe_free_list_heap_impl>;

public:
    using base_t = Base;

    template <typename... Tags>
    static void* allocate(std::size_t size, Tags...)
    {
        assert(size <= sizeof(free_list_node) + Size);
        assert(size >= sizeof(free_list_node));

        auto n = storage::head().data;
        if (!n) {
            auto p = base_t::allocate(Size + sizeof(free_list_node));
            return static_cast<free_list_node*>(p);
        }
        --storage::head().count;
        storage::head().data = n->next;
        return n;
    }

    template <typename... Tags>
    static void deallocate(std::size_t size, void* data, Tags...)
    {
        assert(size <= sizeof(free_list_node) + Size);
        assert(size >= sizeof(free_list_node));

        if (storage::head().count >= Limit)
            base_t::deallocate(Size + sizeof(free_list_node), data);
        else {
            auto n = static_cast<free_list_node*>(data);
            n->next = storage::head().data;
            storage::head().data = n;
            ++storage::head().count;
        }
    }

    static void clear()
    {
        while (storage::head().data) {
            auto n = storage::head().data->next;
            base_t::deallocate(Size + sizeof(free_list_node), storage::head().data);
            storage::head().data = n;
            --storage::head().count;
        }
    }
};

} // namespace detail

/*!
 * Adaptor that does not release the memory to the parent heap but
 * instead it keeps the memory in a global free list that **is not
 * thread-safe**. Must be preceded by a `with_data<free_list_node,
 * ...>` heap adaptor.
 *
 * @tparam Size  Maximum size of the objects to be allocated.
 * @tparam Limit Maximum number of elements to keep in the free list.
 * @tparam Base  Type of the parent heap.
 */
template <std::size_t Size, std::size_t Limit, typename Base>
struct unsafe_free_list_heap : detail::unsafe_free_list_heap_impl<
    detail::unsafe_free_list_storage,
    Size,
    Limit,
    Base>
{};

} // namespace immer
