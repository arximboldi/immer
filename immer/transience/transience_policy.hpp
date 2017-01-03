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

namespace immer {

struct transience_policy
{
    struct owner
    {
        owner* token_ = this;

        owner() {}
        owner(const owner&) {}
        owner(owner&& o) : token_{o.token_} {}
        owner& operator=(const owner&) { return *this; }
        owner& operator=(owner&& o) { token_ = o.token_; return *this; }
    };

    struct ownee
    {
        owner* token_ = nullptr;

        bool can_mutate(const owner& o) const { return token_ == o.token_; }
    };
};

} // namespace immer
