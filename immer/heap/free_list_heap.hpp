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

#include <immer/heap/with_data.hpp>
#include <immer/heap/free_list_node.hpp>

#include <atomic>
#include <cassert>

namespace immer {

/*!
 * Adaptor that does not release the memory to the parent heap but
 * instead it keeps the memory in a thread-safe global free list. Must
 * be preceded by a `with_data<free_list_node, ...>` heap adaptor.
 *
 * @tparam Size Maximum size of the objects to be allocated.
 * @tparam Base Type of the parent heap.
 */
template <std::size_t Size, typename Base>
struct free_list_heap : Base
{
    using base_t = Base;

    template <typename... Tags>
    static void* allocate(std::size_t size, Tags...)
    {
        assert(size <= sizeof(free_list_node) + Size);
        assert(size >= sizeof(free_list_node));

        free_list_node* n;
        do {
            n = head_;
            if (!n) {
                auto p = base_t::allocate(Size + sizeof(free_list_node));
                return static_cast<free_list_node*>(p);
            }
        } while (!head_.compare_exchange_weak(n, n->next));
        return n;
    }

    template <typename... Tags>
    static void deallocate(void* data, Tags...)
    {
        auto n = static_cast<free_list_node*>(data);
        do {
            n->next = head_;
        } while (!head_.compare_exchange_weak(n->next, n));
    }

private:
    using head_t = std::atomic<free_list_node*>;
    static head_t head_;
};

template <std::size_t S, typename B>
typename free_list_heap<S, B>::head_t
free_list_heap<S, B>::head_ {nullptr};

} // namespace immer
