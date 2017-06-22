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

#include <cstdint>

namespace immer {
namespace detail {
namespace rbts {

using bits_t  = std::uint32_t;
using shift_t = std::uint32_t;
using count_t = std::uint32_t;
using size_t  = std::size_t;

template <bits_t B, typename T=count_t>
constexpr T branches = T{1} << B;

template <bits_t B, typename T=size_t>
constexpr T mask = branches<B, T> - 1;

template <bits_t B, bits_t BL>
constexpr shift_t endshift = shift_t{BL} - shift_t{B};

} // namespace rbts
} // namespace detail
} // namespace immer
