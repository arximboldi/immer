//
// immer - immutable data structures for C++
// Copyright (C) 2016, 2017 Juan Pedro Bolivar Puente
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

#include <numeric>
#include <type_traits>

namespace immer {

namespace detail {

template <typename T>
using size_type_t = typename std::decay_t<T>::size_type;

} // namespace detail

// Right now these algorithms dispatch directly to the vector
// implementations unconditionally.  This needs to be reviewed in the
// future because:
//
// 1. We will suport other kinds of containers -- e.g. associative
//    containers, sets, etc.
//
// 2. We should also provide overloads for the iterator based versions
//    of these.  Right now iterator arithmetic has some cost and we
//    might re-evaluate this.

/*!
 * Optimized equivalent of `std::accumulate` applied to the immutable
 * collection `v`.
 */
template <typename VectorT, typename T>
T accumulate(VectorT&& v, T init)
{
    v.for_each_chunk([&] (auto first, auto last) {
        init = std::accumulate(first, last, init);
    });
    return init;
}

/*!
 * Optimized equivalent of `std::accumulate` applied over the subrange
 * @f$ [first, last) @f$ of the immutable collection `v`.
 */
template <typename VectorT, typename T>
T accumulate_i(VectorT&& v,
               detail::size_type_t<VectorT> first,
               detail::size_type_t<VectorT> last,
               T init)
{
    v.for_each_chunk(first, last, [&] (auto first, auto last) {
        init = std::accumulate(first, last, init);
    });
    return init;
}

/*!
 * Optimized equivalent of `std::for_each` applied to the immutable
 * collection `v`.
 */
template <typename VectorT, typename Fn>
Fn&& for_each(VectorT&& v, Fn&& fn)
{
    v.for_each_chunk([&] (auto first, auto last) {
        for (; first != last; ++first)
            fn(*first);
    });
    return std::forward<Fn>(fn);
}

/*!
 * Optimized equivalent of `std::for_each` applied over the subrange
 * @f$ [first, last) @f$ of the immutable collection `v`.
 */
template <typename VectorT, typename Fn>
Fn&& for_each_i(VectorT&& v,
                detail::size_type_t<VectorT> first,
                detail::size_type_t<VectorT> last,
                Fn&& fn)
{
    v.for_each_chunk(first, last, [&] (auto first, auto last) {
        for (; first != last; ++first)
            fn(*first);
    });
    return std::forward<Fn>(fn);
}

} // namespace immer
