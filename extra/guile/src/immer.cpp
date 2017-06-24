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

#include <math.h>
#include <libguile.h>

namespace {

SCM j0_wrapper(SCM x)
{
    return scm_from_double(std::sqrt(scm_to_double(x)));
}

} // anonymous namespace

extern "C" void init_immer ()
{
    scm_c_define_gsubr("j0", 1, 0, 0, (scm_t_subr)j0_wrapper);
}
