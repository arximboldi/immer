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
#include <immer/vector.hpp>
#include <immer/vector_transient.hpp>

#include <catch.hpp>

TEST_CASE("issue-33")
{
    using Element = immer::box<std::string>;
    auto vect = immer::vector<Element>{};

    // this one works fine
    for (auto i = 0; i < 100; ++i) {
        vect = vect.push_back(Element("x"));
    }

    // this one doesn't compile
    auto t = vect.transient();

    for (auto i = 0; i < 100; ++i) {
        t.push_back(Element("x"));
    }

    vect = t.persistent();

    CHECK(*vect[0] == "x");
    CHECK(*vect[99] == "x");
}
