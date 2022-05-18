//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include <catch.hpp>

#ifndef MAP_T
#error "define the map template to use in MAP_T"
#endif

#ifndef MAP_TRANSIENT_T
#error "define the map template to use in MAP_TRANSIENT_T"
#endif

TEST_CASE("instantiate")
{
    auto t = MAP_TRANSIENT_T<std::string, int>{};
    auto m = MAP_T<std::string, int>{};
    CHECK(t.persistent() == m);
    CHECK(t.persistent() == m.transient().persistent());
}
