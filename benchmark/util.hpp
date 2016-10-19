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

#include <utility>
#include <cstddef>
#include <limits>

namespace immer {
template <typename T> struct array;
}

namespace {

struct push_back_fn
{
    template <typename T, typename U>
    auto operator() (T&& v, U&& x)
    {
        return std::forward<T>(v)
            .push_back(std::forward<U>(x));
    }
};

struct push_front_fn
{
    template <typename T, typename U>
    auto operator() (T&& v, U&& x)
    {
        return std::forward<T>(v)
            .push_front(std::forward<U>(x));
    }
};

template <typename T>
struct get_limit : std::integral_constant<
    std::size_t, std::numeric_limits<std::size_t>::max()> {};

template <typename T>
struct get_limit<immer::array<T>> : std::integral_constant<
    std::size_t, 10000> {};

} // anonymous namespace
