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

#include <immer/memory_policy.hpp>
#include <immer/detail/hamts/champ.hpp>

#include <functional>

namespace immer {

/*!
 * **WORK IN PROGRESS**
 */
template <typename K,
          typename T,
          typename Hash          = std::hash<K>,
          typename Equal         = std::equal_to<K>,
          typename MemoryPolicy  = default_memory_policy,
          detail::hamts::bits_t B = default_bits>
class map_transient;

} // namespace immer
