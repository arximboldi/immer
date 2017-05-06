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
#include <immer/vector_transient.hpp>
#include <iostream>
#include <algorithm>

// include:myiota/start
immer::vector<int> myiota(immer::vector<int> v, int first, int last)
{
    auto t = v.transient();
    std::generate_n(std::back_inserter(t),
                    last - first,
                    [&] { return first++; });
    return t.persistent();
}
// include:myiota/end

int main()
{
    auto v = myiota({}, 0, 100);
    std::copy(v.begin(), v.end(),
              std::ostream_iterator<int>{
                  std::cout, "\n"});
}
