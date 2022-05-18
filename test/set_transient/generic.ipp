//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include <catch.hpp>

#ifndef SET_T
#error "define the set template to use in SET_T"
#endif

#ifndef SET_TRANSIENT_T
#error "define the set template to use in SET_TRANSIENT_T"
#endif

TEST_CASE("instantiate")
{
    auto t = SET_TRANSIENT_T<int>{};
    auto m = SET_T<int>{};
    CHECK(t.persistent() == m);
    CHECK(t.persistent() == m.transient().persistent());
}
