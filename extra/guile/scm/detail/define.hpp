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

#pragma once

#ifndef SCM_AUTO_EXPORT
#define SCM_AUTO_EXPORT 1
#endif

#include <scm/detail/subr_wrapper.hpp>
#include <scm/list.hpp>

namespace scm {
namespace detail {

template <typename Tag, typename Fn>
static void define_impl(const std::string& name, Fn fn)
{
    using args_t = function_args_t<Fn>;
    constexpr auto args_size = pack_size_v<args_t>;
    constexpr auto has_rest  = std::is_same<pack_last_t<args_t>, scm::args>{};
    constexpr auto arg_count = args_size - has_rest;
    auto subr = (scm_t_subr) +subr_wrapper_aux<Tag>(fn, args_t{});
    scm_c_define_gsubr(name.c_str(), arg_count, 0, has_rest, subr);
#if SCM_AUTO_EXPORT
    scm_c_export(name.c_str());
#endif
}

} // namespace detail
} // namespace scm
