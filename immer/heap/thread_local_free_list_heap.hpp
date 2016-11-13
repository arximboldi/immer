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
    };

    static head_t head;
};

template <typename Heap>
typename unsafe_free_list_storage<Heap>::head_t
unsafe_free_list_storage<Heap>::head {nullptr};

template <typename Heap>
struct thread_local_free_list_storage
{
    struct head_t
    {
        free_list_node* data;
        ~head_t() { Heap::clear(); }
    };

    thread_local static head_t head;
};

template <typename Heap>
thread_local typename thread_local_free_list_storage<Heap>::head_t
thread_local_free_list_storage<Heap>::head {nullptr};

template <template<class>class Storage,
          std::size_t Size,
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

        free_list_node* n;
        n = storage::head.data;
        if (!n) {
            auto p = base_t::allocate(Size + sizeof(free_list_node));
            return static_cast<free_list_node*>(p);
        }
        storage::head.data = n->next;
        return n;
    }

    template <typename... Tags>
    static void deallocate(void* data, Tags...)
    {
        auto n = static_cast<free_list_node*>(data);
        n->next = storage::head.data;
        storage::head.data = n;
    }

    static void clear()
    {
        while (storage::head.data) {
            auto n = storage::head.data->next;
            base_t::deallocate(storage::head.data);
            storage::head.data = n;
        }
    }
};

} // namespace detail

/*!
 * todo
 */
template <std::size_t Size, typename Base>
struct thread_local_free_list_heap : detail::unsafe_free_list_heap_impl<
    detail::thread_local_free_list_storage,
    Size,
    Base>
{};

/*!
 * todo
 */
template <std::size_t Size, typename Base>
struct unsafe_free_list_heap : detail::unsafe_free_list_heap_impl<
    detail::unsafe_free_list_storage,
    Size,
    Base>
{};

} // namespace immer
