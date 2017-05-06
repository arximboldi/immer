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

#include <memory>

namespace immer {

/*!
 * A heap that uses `operator new` and `operator delete`.
 */
struct cpp_heap
{
    /*!
     * Returns a pointer to a memory region of size `size`, if the
     * allocation was successful, and throws otherwise.
     */
    template <typename... Tags>
    static void* allocate(std::size_t size, Tags...)
    {
        return ::operator new(size);
    }

    /*!
     * Releases a memory region `data` that was previously returned by
     * `allocate`.  One must not use nor deallocate again a memory
     * region that once it has been deallocated.
     */
    static void deallocate(std::size_t size, void* data)
    {
        ::operator delete(data);
    }
};

} // namespace immer
