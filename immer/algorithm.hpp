//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <algorithm>
#include <numeric>
#include <type_traits>

#include <immer/detail/type_traits.hpp>

namespace immer {

/**
 * @defgroup algorithm
 * @{
 */

/*@{*/
// Right now these algorithms dispatch directly to the vector
// implementations unconditionally.  This will be changed in the
// future to support other kinds of containers.

/*!
 * Apply operation `fn` for every contiguous *chunk* of data in the
 * range sequentially.  Each time, `Fn` is passed two `value_type`
 * pointers describing a range over a part of the vector.  This allows
 * iterating over the elements in the most efficient way.
 *
 * @rst
 *
 * .. tip:: This is a low level method. Most of the time, :doc:`other
 *    wrapper algorithms <algorithms>` should be used instead.
 *
 * @endrst
 */
template <typename Range, typename Fn>
void for_each_chunk(const Range& r, Fn&& fn)
{
    r.impl().for_each_chunk(std::forward<Fn>(fn));
}

template <typename Iterator, typename Fn>
void for_each_chunk(const Iterator& first, const Iterator& last, Fn&& fn)
{
    assert(&first.impl() == &last.impl());
    first.impl().for_each_chunk(first.index(), last.index(),
                                std::forward<Fn>(fn));
}

template <typename T, typename Fn>
void for_each_chunk(const T* first, const T* last, Fn&& fn)
{
    std::forward<Fn>(fn)(first, last);
}

/*!
 * Apply operation `fn` for every contiguous *chunk* of data in the
 * range sequentially, until `fn` returns `false`.  Each time, `Fn` is
 * passed two `value_type` pointers describing a range over a part of
 * the vector.  This allows iterating over the elements in the most
 * efficient way.
 *
 * @rst
 *
 * .. tip:: This is a low level method. Most of the time, :doc:`other
 *    wrapper algorithms <algorithms>` should be used instead.
 *
 * @endrst
 */
template <typename Range, typename Fn>
bool for_each_chunk_p(const Range& r, Fn&& fn)
{
    return r.impl().for_each_chunk_p(std::forward<Fn>(fn));
}

template <typename Iterator, typename Fn>
bool for_each_chunk_p(const Iterator& first, const Iterator& last, Fn&& fn)
{
    assert(&first.impl() == &last.impl());
    return first.impl().for_each_chunk_p(first.index(), last.index(),
                                         std::forward<Fn>(fn));
}

template <typename T, typename Fn>
bool for_each_chunk_p(const T* first, const T* last, Fn&& fn)
{
    return std::forward<Fn>(fn)(first, last);
}

/*!
 * Equivalent of `std::accumulate` applied to the range `r`.
 */
template <typename Range, typename T>
T accumulate(Range&& r, T init)
{
    for_each_chunk(r, [&] (auto first, auto last) {
        init = std::accumulate(first, last, init);
    });
    return init;
}

template <typename Range, typename T, typename Fn>
T accumulate(Range&& r, T init, Fn fn)
{
    for_each_chunk(r, [&] (auto first, auto last) {
        init = std::accumulate(first, last, init, fn);
    });
    return init;
}

/*!
 * Equivalent of `std::accumulate` applied to the range @f$ [first,
 * last) @f$.
 */
template <typename Iterator, typename T>
T accumulate(Iterator first, Iterator last, T init)
{
    for_each_chunk(first, last, [&] (auto first, auto last) {
        init = std::accumulate(first, last, init);
    });
    return init;
}

template <typename Iterator, typename T, typename Fn>
T accumulate(Iterator first, Iterator last, T init, Fn fn)
{
    for_each_chunk(first, last, [&] (auto first, auto last) {
        init = std::accumulate(first, last, init, fn);
    });
    return init;
}

/*!
 * An alias to `std::distance`
 */
template <typename Iterator, typename Sentinel,
          std::enable_if_t
          <detail::std_distance_supports_v<Iterator,Sentinel>, bool> = true>
auto distance(Iterator first, Sentinel last)
{
    return std::distance(first, last);
}

/*!
 * Equivalent of the `std::distance` applied to the sentinel-delimited
 * forward range @f$ [first, last) @f$
 */
template <typename Iterator, typename Sentinel,
          std::enable_if_t
          <(!detail::std_distance_supports_v<Iterator,Sentinel>)
           && detail::is_forward_iterator_v<Iterator>
           && detail::compatible_sentinel_v<Iterator,Sentinel>
           && (!detail::is_subtractable_v<Sentinel, Iterator>), bool> = true>
auto distance(Iterator first, Sentinel last)
{
    std::size_t result = 0;
    while (first != last) {
        ++first;
        ++result;
    }
    return result;
}

/*!
 * Equivalent of the `std::distance` applied to the sentinel-delimited
 * random access range @f$ [first, last) @f$
 */
template <typename Iterator, typename Sentinel,
          std::enable_if_t
          <(!detail::std_distance_supports_v<Iterator,Sentinel>)
           && detail::is_forward_iterator_v<Iterator>
           && detail::compatible_sentinel_v<Iterator,Sentinel>
           && detail::is_subtractable_v<Sentinel, Iterator>, bool> = true>
auto distance(Iterator first, Sentinel last)
{
    return last - first;
}

/*!
 * Equivalent of `std::for_each` applied to the range `r`.
 */
template <typename Range, typename Fn>
Fn&& for_each(Range&& r, Fn&& fn)
{
    for_each_chunk(r, [&] (auto first, auto last) {
        for (; first != last; ++first)
            fn(*first);
    });
    return std::forward<Fn>(fn);
}

/*!
 * Equivalent of `std::for_each` applied to the range @f$ [first,
 * last) @f$.
 */
template <typename Iterator, typename Fn>
Fn&& for_each(Iterator first, Iterator last, Fn&& fn)
{
    for_each_chunk(first, last, [&] (auto first, auto last) {
        for (; first != last; ++first)
            fn(*first);
    });
    return std::forward<Fn>(fn);
}

/*!
 * Equivalent of `std::copy` applied to the range `r`.
 */
template <typename Range, typename OutIter>
OutIter copy(Range&& r, OutIter out)
{
    for_each_chunk(r, [&] (auto first, auto last) {
        out = std::copy(first, last, out);
    });
    return out;
}

/*!
 * Equivalent of `std::copy` applied to the range @f$ [first,
 * last) @f$.
 */
template <typename InIter, typename OutIter>
OutIter copy(InIter first, InIter last, OutIter out)
{
    for_each_chunk(first, last, [&] (auto first, auto last) {
        out = std::copy(first, last, out);
    });
    return out;
}

/*!
 * Equivalent of `std::all_of` applied to the range `r`.
 */
template <typename Range, typename Pred>
bool all_of(Range&& r, Pred p)
{
    return for_each_chunk_p(r, [&] (auto first, auto last) {
        return std::all_of(first, last, p);
    });
}

/*!
 * Equivalent of `std::all_of` applied to the range @f$ [first, last)
 * @f$.
 */
template <typename Iter, typename Pred>
bool all_of(Iter first, Iter last, Pred p)
{
    return for_each_chunk_p(first, last, [&] (auto first, auto last) {
        return std::all_of(first, last, p);
    });
}

/*!
 * An alias to `std::uninitialized_copy`
 */
template <typename Iterator, typename Sentinel, typename SinkIter,
          std::enable_if_t
          <detail::std_uninitialized_copy_supports_v
           <Iterator,Sentinel,SinkIter>, bool> = true>
auto uninitialized_copy(Iterator first, Sentinel last, SinkIter d_first)
{
    return std::uninitialized_copy(first, last, d_first);
}

/*!
 * Equivalent of the `std::uninitialized_copy` applied to the
 * sentinel-delimited forward range @f$ [first, last) @f$
 */
template <typename SourceIter, typename Sent, typename SinkIter,
          std::enable_if_t
          <(!detail::std_uninitialized_copy_supports_v<SourceIter, Sent, SinkIter>)
           && detail::compatible_sentinel_v<SourceIter,Sent>
           && detail::is_forward_iterator_v<SinkIter>, bool> = true>
auto uninitialized_copy(SourceIter first, Sent last, SinkIter d_first)
{
    auto current = d_first;
    try {
        while (first != last) {
            *current++ = *first;
            ++first;
        }
    } catch (...) {
        using Value = typename std::iterator_traits<SinkIter>::value_type;
        for (;d_first != current; ++d_first){
          d_first->~Value();
        }
        throw;
    }
    return current;
}

/** @} */ // group: algorithm

} // namespace immer
