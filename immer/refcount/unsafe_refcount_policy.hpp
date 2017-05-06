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

#include <immer/refcount/no_refcount_policy.hpp>

#include <atomic>
#include <utility>

namespace immer {

/*!
 * A reference counting policy implemented using a raw `int` count.
 * It is **not thread-safe**.
 */
struct unsafe_refcount_policy
{
    mutable int refcount;

    unsafe_refcount_policy() : refcount{1} {};
    unsafe_refcount_policy(disowned) : refcount{0} {}

    void inc() { ++refcount; }
    bool dec() { return --refcount == 0; }
    void dec_unsafe() { --refcount; }
    bool unique() { return refcount == 1; }
};

} // namespace immer
