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

// Adapted from the official std::invoke proposal:
// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n4169.html

#include <type_traits>
#include <functional>

namespace scm {
namespace detail {

template <typename Functor, typename... Args>
std::enable_if_t<
    std::is_member_pointer<std::decay_t<Functor>>::value,
    std::result_of_t<Functor&&(Args&&...)>>
invoke(Functor&& f, Args&&... args)
{
    return std::mem_fn(f)(std::forward<Args>(args)...);
}

template <typename Functor, typename... Args>
std::enable_if_t<
    !std::is_member_pointer<std::decay_t<Functor>>::value,
    std::result_of_t<Functor&&(Args&&...)>>
invoke(Functor&& f, Args&&... args)
{
    return std::forward<Functor>(f)(std::forward<Args>(args)...);
}

} // namespace detail
} // namespace scm
