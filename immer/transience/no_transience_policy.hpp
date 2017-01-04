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

struct no_transience_policy
{
    template <typename>
    struct apply
    {
        struct type
        {
            struct edit {};

            struct owner
            {
                operator edit () const { return {}; }
            };

            struct ownee
            {
                ownee& operator=(edit) { return *this; };
                bool can_mutate(edit) const { return false; }
            };
        };
    };
};

} // namespace immer
