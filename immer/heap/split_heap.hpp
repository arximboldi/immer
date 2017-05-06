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

#include <atomic>
#include <cassert>

namespace immer {

/*!
 * Adaptor that uses `SmallHeap` for allocations that are smaller or
 * equal to `Size` and `BigHeap` otherwise.
 */
template <std::size_t Size, typename SmallHeap, typename BigHeap>
struct split_heap
{
    template <typename... Tags>
    static void* allocate(std::size_t size, Tags... tags)
    {
        return size <= Size
            ? SmallHeap::allocate(size, tags...)
            : BigHeap::allocate(size, tags...);
    }

    template <typename... Tags>
    static void deallocate(std::size_t size, void* data, Tags... tags)
    {
        if (size <= Size)
            SmallHeap::deallocate(size, data, tags...);
        else
            BigHeap::deallocate(size, data, tags...);
    }
};

} // namespace immer
