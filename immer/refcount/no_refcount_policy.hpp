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

namespace immer {

struct disowned {};

/*!
 * Disables reference counting, to be used with an alternative garbage
 * collection strategy like a `gc_heap`.
 */
struct no_refcount_policy
{
    no_refcount_policy() {};
    no_refcount_policy(disowned) {}

    void inc() {}
    bool dec() { return false; }
    void dec_unsafe() {}
    bool unique() { return false; }
};

} // namespace immer
