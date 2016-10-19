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

namespace immer {

template <typename Deriv, typename RefcountPolicy>
struct enable_intrusive_ptr : RefcountPolicy::data
{
    using policy = RefcountPolicy;

    enable_intrusive_ptr()
        : policy::data{disowned{}}
    {}

    friend void intrusive_ptr_add_ref(const Deriv* x)
    {
        policy::inc(x);
    }

    friend void intrusive_ptr_release(const Deriv* x)
    {
        if (policy::dec(x))
            delete x;
    }
};

} // namespace immer
