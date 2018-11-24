//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <cstdint>

#if defined(_MSC_VER)
#include <intrin.h> // __popcnt
#endif

namespace immer {
namespace detail {
namespace hamts {

using size_t   = std::size_t;
using hash_t   = std::size_t;

template <size_t B>
struct types
{
    static_assert(B < 6, "B > 6 is not supported.");

    using bits_t   = std::uint32_t;
    using bitmap_t = std::uint32_t;
    using count_t  = std::uint32_t;
    using shift_t  = std::uint32_t;
};

template <>
struct types<6>
{
    using bits_t   = std::uint64_t;
    using bitmap_t = std::uint64_t;
    using count_t  = std::uint64_t;
    using shift_t  = std::uint64_t;
};

template <size_t B>
using bits_t   = typename types<B>::bits_t;

template <size_t B>
using bitmap_t = typename types<B>::bitmap_t;

template <size_t B>
using count_t  = typename types<B>::count_t;

template <size_t B>
using shift_t  = typename types<B>::shift_t;

template <size_t B, typename T=count_t<B>>
constexpr T branches = T{1} << B;

template <size_t B, typename T=size_t>
constexpr T mask = branches<B, T> - 1;

template <size_t B, typename T=count_t<B>>
constexpr T max_depth = (sizeof(hash_t) * 8 + B - 1) / B;

template <size_t B, typename T=count_t<B>>
constexpr T max_shift = max_depth<B, count_t<B>> * B;

#define IMMER_HAS_BUILTIN_POPCOUNT 1

template <size_t B>
inline auto popcount_fallback(bitmap_t<B> x)
{
    // More alternatives:
    // https://en.wikipedia.org/wiki/Hamming_weight
    // http://wm.ite.pl/articles/sse-popcount.html
    // http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
    x = x - ((x >> 1) & 0x55555555);
    x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
    return ((x + (x >> 4) & 0xF0F0F0F) * 0x1010101) >> 24;
}

template <size_t B>
inline count_t<B> popcount(bitmap_t<B> x)
{
#if IMMER_HAS_BUILTIN_POPCOUNT
#  if defined(_MSC_VER)
    return  __popcnt(x);
#  else
    return __builtin_popcount(x);
#  endif
#else
    return popcount_fallback(x);
#endif
}

inline count_t<6> popcount(bitmap_t<6> x)
{
#if IMMER_HAS_BUILTIN_POPCOUNT
#  if defined(_MSC_VER)
    return  __popcnt64(x);
#  else
    return __builtin_popcountll(x);
#  endif
#else
    return popcount_fallback(x);
#endif
}

} // namespace hamts
} // namespace detail
} // namespace immer
