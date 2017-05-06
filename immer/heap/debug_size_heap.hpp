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
#include <immer/heap/identity_heap.hpp>
#include <cassert>

namespace immer {

#if IMMER_ENABLE_DEBUG_SIZE_HEAP

/*!
 * A heap that in debug mode ensures that the sizes for allocation and
 * deallocation do match.
 */
template <typename Base>
struct debug_size_heap
{
    template <typename... Tags>
    static void* allocate(std::size_t size, Tags... tags)
    {
        auto p = (std::size_t*) Base::allocate(size + sizeof(std::size_t), tags...);
        *p = size;
        return p + 1;
    }

    template <typename... Tags>
    static void deallocate(std::size_t size, void* data, Tags... tags)
    {
        auto p = ((std::size_t*) data) - 1;
        assert(*p == size);
        Base::deallocate(size + sizeof(std::size_t), p, tags...);
    }
};

#else // IMMER_ENABLE_DEBUG_SIZE_HEAP

template <typename Base>
using debug_size_heap = identity_heap<Base>;

#endif // !IMMER_ENABLE_DEBUG_SIZE_HEAP

} // namespace immer
