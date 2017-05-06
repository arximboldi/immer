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

namespace immer {

template <typename Deriv, typename RefcountPolicy>
class enable_intrusive_ptr
{
    mutable RefcountPolicy refcount_data_;

public:
    enable_intrusive_ptr()
        : refcount_data_{disowned{}}
    {}

    friend void intrusive_ptr_add_ref(const Deriv* x)
    {
        x->refcount_data_.inc();
    }

    friend void intrusive_ptr_release(const Deriv* x)
    {
        if (x->refcount_data_.dec())
            delete x;
    }
};

} // namespace immer
