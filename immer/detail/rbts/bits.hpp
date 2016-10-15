
#pragma once

#include <cstddef>

namespace immer {
namespace detail {
namespace rbts {

using bits_t  = unsigned;
using shift_t = unsigned;
using count_t = unsigned;
using size_t  = std::size_t;

template <bits_t B, typename T=size_t>
constexpr T branches = T{1} << B;

template <bits_t B, typename T=size_t>
constexpr T mask = branches<B, T> - 1;

} // namespace rbts
} // namespace detail
} // namespace immer
