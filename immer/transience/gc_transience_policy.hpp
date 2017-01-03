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

#include <memory>

namespace immer {

struct gc_transience_policy
{
    template <typename HeapPolicy>
    struct apply
    {
        struct type
        {
            using heap = typename HeapPolicy::template apply<1>::type;

            struct owner
            {
                void* token_ = heap::allocate(1, norefs_tag{});

                owner() {}
                owner(const owner&) {}
                owner(owner&& o) : token_{o.token_} {}
                owner& operator=(const owner&) { return *this; }
                owner& operator=(owner&& o)
                {
                    token_ = o.token_;
                    return *this;
                }
            };

            struct ownee
            {
                void* token_ = nullptr;

                void set(const owner& o) { token_ = o; }

                bool can_mutate(const owner& o) const
                { return token_ == o.token_; }
            };
        };
    };
};

} // namespace immer
