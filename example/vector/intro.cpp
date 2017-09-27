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

// include:intro/start
#include <immer/vector.hpp>
int main()
{
    const auto v0 = immer::vector<int>{};
    const auto v1 = v0.push_back(13);
    assert((v0 == immer::vector<int>{}));
    assert((v1 == immer::vector<int>{13}));

    const auto v2 = v1.set(0, 42);
    assert(v1[0] == 13);
    assert(v2[0] == 42);
}
// include:intro/end
