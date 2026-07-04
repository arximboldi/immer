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

namespace immer {
namespace detail {
namespace bts {

using size_t  = std::size_t;
using bits_t  = std::uint32_t;
using count_t = std::uint32_t;

// Type of the per-child cumulative subtree sizes stored in inner
// nodes.  Being 32 bits, it caps the number of elements a tree can
// hold at 2^32 - 1.
using local_size_t = std::uint32_t;

template <bits_t B, typename T = count_t>
constexpr T branches = T{1u} << B;

template <bits_t B, typename T = count_t>
constexpr T min_branches = branches<B, T> / T{2u};

// Upper bound for the number of levels of a tree.  Since local_size_t
// bounds the tree size by 2^32 and every non-root node has at least
// min_branches<B> >= 2 children, this suffices for iterator stacks
// and recursion depths regardless of BL.
template <bits_t B, typename T = count_t>
constexpr T max_depth = T{(32u + B - 2u) / (B - 1u) + 1u};

} // namespace bts
} // namespace detail
} // namespace immer
