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

#include <cstdio>

namespace immer {

/*!
 * Appends a default constructed extra object of type `T` at the
 * *before* the requested region.
 *
 * @tparam T Type of the appended data.
 * @tparam Base Type of the parent heap.
 */
template <typename T, typename Base>
struct with_data : Base
{
    using base_t = Base;

    template <typename... Tags>
    static void* allocate(std::size_t size, Tags... tags)
    {
        auto p = base_t::allocate(size + sizeof(T), tags...);
        return new (p) T{} + 1;
    }

    template <typename... Tags>
    static void deallocate(std::size_t size, void* p, Tags... tags)
    {
        auto dp = static_cast<T*>(p) - 1;
        dp->~T();
        base_t::deallocate(size + sizeof(T), dp, tags...);
    }
};

} // namespace immer
