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

#include <string>
// include:intro/start
#include <immer/map.hpp>
int main()
{
    const auto v0 = immer::map<std::string, int>{};
    const auto v1 = v0.set("hello", 42);
    assert(v0["hello"] == 0);
    assert(v1["hello"] == 42);

    const auto v2 = v1.erase("hello");
    assert(*v1.find("hello") == 42);
    assert(!v2.find("hello"));
}
// include:intro/end
