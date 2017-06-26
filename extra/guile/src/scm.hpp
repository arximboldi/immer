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

#include <libguile.h>
#include <type_traits>
#include <utility>
#include <cstdint>

namespace scm {

namespace detail {

template <typename T>
struct type_meta;

template <>
struct type_meta<SCM>
{
    struct convert
    {
        static SCM to_cpp(SCM v) { return v; }
        static SCM to_scm(SCM v) { return v; }
    };
};

} // namespace detail

template <typename T>
SCM to_scm(T&& v)
{
    using meta_t = detail::type_meta<std::decay_t<T>>;
    return meta_t::convert::to_scm(std::forward<T>(v));
}

template <typename T>
T to_cpp(SCM v)
{
    using meta_t = detail::type_meta<std::decay_t<T>>;
    return meta_t::convert::to_cpp(v);
}

} // namespace scm

#define SCM_DECLARE_NUMERIC_TYPE(cpp_name__, scm_name__)                \
    namespace scm {                                                     \
    namespace detail {                                                  \
    template <>                                                         \
    struct type_meta<cpp_name__> {                                      \
        struct convert {                                                \
            static cpp_name__ to_cpp(SCM v) { return scm_to_ ## scm_name__(v); } \
            static SCM to_scm(cpp_name__ v) { return scm_from_ ## scm_name__(v); } \
        };                                                              \
    };                                                                  \
    }} /* namespace scm::detail */                                      \
    /**/

SCM_DECLARE_NUMERIC_TYPE(float,         double);
SCM_DECLARE_NUMERIC_TYPE(double,        double);
SCM_DECLARE_NUMERIC_TYPE(std::int8_t,   int8);
SCM_DECLARE_NUMERIC_TYPE(std::int16_t,  int16);
SCM_DECLARE_NUMERIC_TYPE(std::int32_t,  int32);
SCM_DECLARE_NUMERIC_TYPE(std::int64_t,  int64);
SCM_DECLARE_NUMERIC_TYPE(std::uint8_t,  uint8);
SCM_DECLARE_NUMERIC_TYPE(std::uint16_t, uint16);
SCM_DECLARE_NUMERIC_TYPE(std::uint32_t, uint32);
SCM_DECLARE_NUMERIC_TYPE(std::uint64_t, uint64);
