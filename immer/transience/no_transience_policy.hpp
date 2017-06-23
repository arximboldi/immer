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

/*!
 * Disables any special *transience* tracking.  To be used when
 * *reference counting* is available instead.
 */
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
                owner& operator=(const owner&) { return *this; };
            };

            struct ownee
            {
                ownee& operator=(edit) { return *this; };
                bool can_mutate(edit) const { return false; }
                bool owned() const { return false; }
            };

            static owner noone;
        };
    };
};

template <typename HP>
typename no_transience_policy::apply<HP>::type::owner
no_transience_policy::apply<HP>::type::noone = {};

} // namespace immer
