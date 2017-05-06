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

#include <immer/box.hpp>
#include <cassert>
#include <string>

int main()
{
    {
// include:update/start
        auto v1 = immer::box<std::string>{"hello"};
        auto v2 = v1.update([&] (auto l) {
            return l + ", world!";
        });

        assert((v1 == immer::box<std::string>{"hello"}));
        assert((v2 == immer::box<std::string>{"hello, world!"}));
// include:update/end
    }
}
