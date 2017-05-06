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
#include <string>
#include <iostream>

// include:fizzbuzz/start
immer::vector<std::string> fizzbuzz(immer::vector<std::string> v, int first, int last)
{
    for (auto i = first; i < last; ++i)
        v = std::move(v).push_back(
            i % 15 == 0 ? "FizzBuzz" :
            i % 5  == 0 ? "Bizz" :
            i % 3  == 0 ? "Fizz" :
            /* else */    std::to_string(i));
    return v;
}
// include:fizzbuzz/end

int main()
{
    auto v = fizzbuzz({}, 0, 100);
    std::copy(v.begin(), v.end(),
              std::ostream_iterator<std::string>{
                  std::cout, "\n"});
}
