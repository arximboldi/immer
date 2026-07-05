//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include <cassert>
// include:intro/start
#include <immer/sorted_set.hpp>
int main()
{
    const auto v0 = immer::sorted_set<int>{3, 1, 2};
    assert(v0.front() == 1);
    assert(v0.back() == 3);

    const auto v1 = v0.insert(0);
    assert(v1.front() == 0);
    assert(v0.front() == 1);

    // ranges between bounds can be traversed
    auto sum = 0;
    for (auto it = v1.lower_bound(1), e = v1.upper_bound(2); it != e; ++it)
        sum += *it;
    assert(sum == 3);
}
// include:intro/end
