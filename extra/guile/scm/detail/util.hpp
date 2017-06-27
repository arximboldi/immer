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

#include <libguile.h>

namespace scm {
namespace detail {

#define SCM_DECLTYPE_RETURN(...)                \
    decltype(__VA_ARGS__)                       \
    { return __VA_ARGS__; }                     \
    /**/

template <typename... Ts>
constexpr bool is_valid_v = true;

template <typename... Ts>
using is_valid_t = void;

template <typename... Ts>
void check_call_once()
{
    static bool called = false;
    if (called) scm_misc_error (nullptr, "Double defined binding. \
This may be caused because there are multiple C++ binding groups in the same \
translation unit.  You may solve this by using different type tags for each \
binding group.", SCM_EOL);
    called = true;
}

struct move_sequence
{
    move_sequence() = default;
    move_sequence(const move_sequence&) = delete;
    move_sequence(move_sequence&& other)
    { other.moved_from_ = true; };

    bool moved_from_ = false;
};

} // namespace detail
} // namespace scm
