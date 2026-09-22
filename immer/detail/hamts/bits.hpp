//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <cstddef>
#include <cstdint>

#if defined(_MSC_VER)
#include <intrin.h> // __popcnt, __cpuid
#endif

namespace immer {
namespace detail {
namespace hamts {

using size_t  = std::size_t;
using bits_t  = std::uint32_t;
using count_t = std::uint32_t;
using shift_t = std::uint32_t;

template <bits_t B>
struct get_bitmap_type
{
    static_assert(B < 6u, "B > 6 is not supported.");
    using type = std::uint8_t;
};

template <>
struct get_bitmap_type<6u>
{
    using type = std::uint64_t;
};

template <>
struct get_bitmap_type<5u>
{
    using type = std::uint32_t;
};

template <>
struct get_bitmap_type<4u>
{
    using type = std::uint16_t;
};

template <bits_t B, typename T = count_t>
constexpr T branches = T{1u} << B;

template <typename T, bits_t B>
constexpr T mask = branches<B, T> - 1u;

template <typename hash_t, bits_t B, typename T = count_t>
constexpr T max_depth = (sizeof(hash_t) * 8u + B - 1u) / B;

template <typename hash_t, bits_t B, typename T = count_t>
constexpr T max_shift = max_depth<hash_t, B, count_t> * B;

// __popcnt/__popcnt64 compile directly to the x86 POPCNT instruction, which
// only exists from Intel's Nehalem generation (2008) onward (and AMD
// equivalents) and has no compiler-inserted CPUID check, unlike
// __builtin_popcount on GCC/Clang. MSVC only exposes a POPCNT capability
// query (via CPUID) on x86/x64; other MSVC targets (e.g. ARM64) have no
// popcount intrinsic to speak of, so there's nothing to "have" there --
// always use the portable fallback on such targets.
#if defined(_MSC_VER) && !(defined(_M_IX86) || defined(_M_X64))
#define IMMER_HAS_BUILTIN_POPCOUNT 0
#else
#define IMMER_HAS_BUILTIN_POPCOUNT 1
#endif

#if IMMER_HAS_BUILTIN_POPCOUNT && defined(_MSC_VER)
inline bool has_popcnt_support()
{
    static const bool result = [] {
        int info[4] = {0, 0, 0, 0};
        __cpuid(info, 1);
        return (info[2] & (1 << 23)) != 0; // ECX bit 23 == POPCNT
    }();
    return result;
}
#endif

inline auto popcount_fallback(std::uint32_t x)
{
    // More alternatives:
    // https://en.wikipedia.org/wiki/Hamming_weight
    // http://wm.ite.pl/articles/sse-popcount.html
    // http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
    x = x - ((x >> 1) & 0x55555555u);
    x = (x & 0x33333333u) + ((x >> 2) & 0x33333333u);
    return (((x + (x >> 4u)) & 0xF0F0F0Fu) * 0x1010101u) >> 24u;
}

inline auto popcount_fallback(std::uint64_t x)
{
    x = x - ((x >> 1) & 0x5555555555555555u);
    x = (x & 0x3333333333333333u) + ((x >> 2u) & 0x3333333333333333u);
    return (((x + (x >> 4)) & 0x0F0F0F0F0F0F0F0Fu) * 0x0101010101010101u) >>
           56u;
}

inline count_t popcount(std::uint32_t x)
{
#if !IMMER_HAS_BUILTIN_POPCOUNT
    return popcount_fallback(x);
#elif defined(_MSC_VER)
    return has_popcnt_support() ? __popcnt(x) : popcount_fallback(x);
#else
    return __builtin_popcount(x);
#endif
}

inline count_t popcount(std::uint64_t x)
{
#if !IMMER_HAS_BUILTIN_POPCOUNT
    return popcount_fallback(x);
#elif defined(_MSC_VER)
#if defined(_WIN64)
    return has_popcnt_support() ? static_cast<count_t>(__popcnt64(x))
                                 : popcount_fallback(x);
#else
    // TODO: benchmark against popcount_fallback(std::uint64_t x)
    // Safe on CPUs without POPCNT: popcount(std::uint32_t) already checks
    // has_popcnt_support() before using the intrinsic.
    return popcount(static_cast<std::uint32_t>(x >> 32)) +
           popcount(static_cast<std::uint32_t>(x));
#endif
#else
    return __builtin_popcountll(x);
#endif
}

inline count_t popcount(std::uint16_t x)
{
    return popcount(static_cast<std::uint32_t>(x));
}

inline count_t popcount(std::uint8_t x)
{
    return popcount(static_cast<std::uint32_t>(x));
}

template <typename bitmap_t>
class set_bits_range
{
    bitmap_t bitmap;

    class set_bits_iterator
    {
        bitmap_t bitmap;

        inline static bitmap_t clearlsbit(bitmap_t bitmap)
        {
            return bitmap & (bitmap - 1);
        }

        inline static bitmap_t lsbit(bitmap_t bitmap)
        {
            return bitmap ^ clearlsbit(bitmap);
        }

    public:
        set_bits_iterator(bitmap_t bitmap)
            : bitmap(bitmap){};

        set_bits_iterator operator++()
        {
            bitmap = clearlsbit(bitmap);
            return *this;
        }

        bool operator!=(set_bits_iterator const& other) const
        {
            return bitmap != other.bitmap;
        }

        bitmap_t operator*() const { return lsbit(bitmap); }
    };

public:
    set_bits_range(bitmap_t bitmap)
        : bitmap(bitmap)
    {
    }
    set_bits_iterator begin() const { return set_bits_iterator(bitmap); }
    set_bits_iterator end() const { return set_bits_iterator(0); }
};

} // namespace hamts
} // namespace detail
} // namespace immer
