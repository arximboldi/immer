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

#include <immer/vector.hpp>
#include <cassert>

// include:move-bad/start
immer::vector<int> do_stuff(const immer::vector<int> v)
{
    return std::move(v).push_back(42);
}
// include:move-bad/end

// include:move-good/start
immer::vector<int> do_stuff_better(immer::vector<int> v)
{
    return std::move(v).push_back(42);
}
// include:move-good/end

int main()
{
    auto v = immer::vector<int>{};
    auto v1 = do_stuff(v);
    auto v2 = do_stuff_better(v);
    assert(v1.size() == 1);
    assert(v2.size() == 1);
    assert(v1[0] == 42);
    assert(v2[0] == 42);
}
