//
// immer - immutable data structures for C++
// Copyright (C) 2016 Juan Pedro Bolivar Puente
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

#include <immer/refcount/no_refcount_policy.hpp>

#include <atomic>
#include <utility>

namespace immer {

struct refcount_policy
{
    static constexpr bool enabled = true;

    struct data
    {
        mutable std::atomic<int> refcount;

        data() : refcount{1} {};
        data(disowned) : refcount{0} {}
    };

    static void inc(const data* p)
    {
        p->refcount.fetch_add(1, std::memory_order_relaxed);
    };

    static bool dec(const data* p)
    {
        return 1 == p->refcount.fetch_sub(1, std::memory_order_acq_rel);
    };

    template <typename T>
    static void dec_unsafe(const T* p)
    {
        assert(p->refcount.load() > 1);
        p->refcount.fetch_sub(1, std::memory_order_relaxed);
    };
};

} // namespace immer
