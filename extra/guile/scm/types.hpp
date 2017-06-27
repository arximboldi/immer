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
#include <cstdint>
#include <utility>

namespace scm {

using val = SCM;

namespace detail {

template <typename T>
struct foreign_type_storage
{
    static val data;
};

template <typename T>
val foreign_type_storage<T>::data = SCM_UNSPECIFIED;

template <typename T>
struct convert;

template <typename T>
struct convert_wrapper_type
{
    static T to_cpp(val v) { return v; }
    static val to_scm(T v) { return v; }
};

template <typename T>
struct convert_foreign_type
{
    using storage_t = foreign_type_storage<T>;
    static T& to_cpp(val v)
    {
        scm_assert_foreign_object_type(storage_t::data, v);
        return *(T*)scm_foreign_object_ref(v, 0);
    }

    template <typename U>
    static val to_scm(U&& v)
    {
        return scm_make_foreign_object_1(
            storage_t::data,
            new (scm_gc_malloc(sizeof(T), "scmpp")) T{
                std::forward<U>(v)});
    }
};

} // namespace detail

template <typename T>
val to_scm(T&& v)
{
    return detail::convert<std::decay_t<T>>::to_scm(std::forward<T>(v));
}

template <typename T>
T to_cpp(val v)
{
    return detail::convert<std::decay_t<T>>::to_cpp(v);
}

template <typename T=val>
T call(val fn)
{
    return to_cpp<T>(scm_call_0(fn));
}
template <typename T=val, typename T0>
T call(val fn, T0&& a0)
{
    return to_cpp<T>(scm_call_1(fn, to_scm(std::forward<T0>(a0))));
}
template <typename T=val, typename T0, typename T1>
T call(val fn, T0&& a0, T1&& a1)
{
    return to_cpp<T>(scm_call_2(fn,
                                to_scm(std::forward<T0>(a0)),
                                to_scm(std::forward<T1>(a1))));
}
template <typename T=val, typename T0, typename T1, typename T2>
T call(val fn, T0&& a0, T1&& a1, T2&& a2)
{
    return to_cpp<T>(scm_call_2(fn,
                                to_scm(std::forward<T0>(a0)),
                                to_scm(std::forward<T1>(a1)),
                                to_scm(std::forward<T2>(a2))));
}

} // namespace scm

#define SCM_DECLARE_FOREIGN_TYPE(cpp_name__)                            \
    namespace scm {                                                     \
    namespace detail {                                                  \
    template <>                                                         \
    struct convert<cpp_name__>                                          \
        : convert_foreign_type<cpp_name__> {};                          \
    }} /* namespace scm::detail */                                      \
    /**/

#define SCM_DECLARE_NUMERIC_TYPE(cpp_name__, scm_name__)                \
    namespace scm {                                                     \
    namespace detail {                                                  \
    template <>                                                         \
    struct convert<cpp_name__> {                                        \
        static cpp_name__ to_cpp(SCM v) { return scm_to_ ## scm_name__(v); } \
        static SCM to_scm(cpp_name__ v) { return scm_from_ ## scm_name__(v); } \
    };                                                                  \
    }} /* namespace scm::detail */                                      \
    /**/

#define SCM_DECLARE_WRAPPER_TYPE(cpp_name__)                            \
    namespace scm {                                                     \
    namespace detail {                                                  \
    template <>                                                         \
    struct convert<cpp_name__>                                          \
        : convert_wrapper_type<cpp_name__> {};                          \
    }} /* namespace scm::detail */                                      \
    /**/

SCM_DECLARE_WRAPPER_TYPE(SCM);

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
