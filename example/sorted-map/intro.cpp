//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include <cassert>
#include <string>
// include:intro/start
#include <immer/sorted_map.hpp>
int main()
{
    const auto v0 = immer::sorted_map<int, std::string>{};
    const auto v1 = v0.set(12, "twelve").set(4, "four").set(8, "eight");
    assert(v1.size() == 3);
    assert(v1.at(8) == "eight");

    // iteration happens in key order
    assert(v1.begin()->first == 4);
    assert(v1.back().first == 12);

    // and we can query by order
    assert(v1.lower_bound(5)->first == 8);

    const auto v2 = v1.erase(8);
    assert(v1.count(8) == 1);
    assert(v2.count(8) == 0);
}
// include:intro/end
