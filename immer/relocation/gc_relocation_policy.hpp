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
 * Enables relocation information tracking; used when a relocating garbage
 * collector is present in the final system.
 */
struct gc_relocation_policy
{
    gc_relocation_policy(){};
};

} // namespace immer
