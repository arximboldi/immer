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

#include <scm/detail/util.hpp>

#include <cstdint>
#include <utility>

#include <libguile.h>

namespace scm {
namespace detail {

template <typename T>
struct foreign_type_storage
{
    static SCM data;
};

template <typename T>
SCM foreign_type_storage<T>::data = SCM_UNSPECIFIED;

template <typename T>
struct convert;

template <typename T>
struct convert_wrapper_type
{
    static T to_cpp(SCM v) { return T{v}; }
    static SCM to_scm(T v) { return v.get(); }
};

template <typename T>
struct convert_foreign_type
{
    using storage_t = foreign_type_storage<T>;
    static T& to_cpp(SCM v)
    {
        scm_assert_foreign_object_type(storage_t::data, v);
        return *(T*)scm_foreign_object_ref(v, 0);
    }

    template <typename U>
    static SCM to_scm(U&& v)
    {
        return scm_make_foreign_object_1(
            storage_t::data,
            new (scm_gc_malloc(sizeof(T), "scmpp")) T{
                std::forward<U>(v)});
    }
};

template <typename T>
auto to_scm(T&& v)
    -> SCM_DECLTYPE_RETURN(
        detail::convert<std::decay_t<T>>::to_scm(std::forward<T>(v)));

template <typename T>
auto to_cpp(SCM v)
    -> SCM_DECLTYPE_RETURN(
        detail::convert<std::decay_t<T>>::to_cpp(v));

struct wrapper
{
    wrapper() = default;
    wrapper(SCM hdl) : handle_{hdl} {}
    SCM get() const { return handle_; }
    operator SCM () const { return handle_; }

    bool operator==(wrapper other) { return handle_ == other.handle_; }
    bool operator!=(wrapper other) { return handle_ != other.handle_; }

protected:
    SCM handle_ = SCM_UNSPECIFIED;
};

} // namespace detail

struct val : detail::wrapper
{
    using base_t = detail::wrapper;
    using base_t::base_t;

    template <typename T,
              typename Enable=std::enable_if_t<
                  (!std::is_same<std::decay_t<T>, val>{} &&
                   !std::is_same<std::decay_t<T>, SCM>{})>>
    explicit val(T&& x)
        : base_t(detail::to_scm(std::forward<T>(x)))
    {}

    template <typename T,
              typename Enable=detail::is_valid_t<
                  decltype(static_cast<T>(detail::to_cpp<T>(SCM{})))>>
    operator T() const { return detail::to_cpp<T>(handle_); }
    template <typename T,
              typename Enable=detail::is_valid_t<
                  decltype(static_cast<T&>(detail::to_cpp<T>(SCM{})))>>
    operator T&() { return detail::to_cpp<T>(handle_); }
    template <typename T,
              typename Enable=detail::is_valid_t<
                  decltype(static_cast<const T&>(detail::to_cpp<T>(SCM{})))>>
    operator const T&() { return detail::to_cpp<T>(handle_); }

    val operator() () const
    { return val{scm_call_0(get())}; }
    val operator() (val a0) const
    { return val{scm_call_1(get(), a0)}; }
    val operator() (val a0, val a1) const
    { return val{scm_call_2(get(), a0, a1)}; }
    val operator() (val a0, val a1, val a3) const
    { return val{scm_call_3(get(), a0, a1, a3)}; }
};

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

SCM_DECLARE_WRAPPER_TYPE(val);

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
