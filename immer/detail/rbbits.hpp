
#pragma once

#include <cstddef>

namespace immer {
namespace detail {

template <int B, typename T=std::size_t>
constexpr T branches = T{1} << B;

template <int B, typename T=std::size_t>
constexpr T mask = branches<B, T> - 1;

} // namespace detail
} // namespace immer
