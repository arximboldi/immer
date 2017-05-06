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

#include <atomic>

namespace immer {
namespace detail {

template <typename Deriv>
struct ref_count_base
{
    mutable std::atomic<int> ref_count { 0 };

    friend void intrusive_ptr_add_ref(const Deriv* x)
    {
        x->ref_count.fetch_add(1, std::memory_order_relaxed);
    }

    friend void intrusive_ptr_release(const Deriv* x)
    {
        if (x->ref_count.fetch_sub(1, std::memory_order_release) == 1) {
            std::atomic_thread_fence(std::memory_order_acquire);
            delete x;
        }
    }
};

} /* namespace detail */
} /* namespace immer */
