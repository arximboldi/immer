//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

namespace immer {

/*!
 * Disables relocation information tracking; used when a relocating garbage
 * collector is not present in the final system.
 */
struct no_relocation_policy
{
    no_relocation_policy(){};
};

} // namespace immer
