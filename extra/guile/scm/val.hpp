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

#include <scm/detail/convert.hpp>

namespace scm {
namespace detail {

template <typename T>
struct convert_wrapper_type
{
    static T to_cpp(SCM v) { return T{v}; }
    static SCM to_scm(T v) { return v.get(); }
};

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
              typename = std::enable_if_t<
                  (!std::is_same<std::decay_t<T>, val>{} &&
                   !std::is_same<std::decay_t<T>, SCM>{})>>
    val(T&& x)
        : base_t(detail::to_scm(std::forward<T>(x)))
    {}

    template <typename T,
              typename = std::enable_if_t<
                  std::is_same<T, decltype(detail::to_cpp<T>(SCM{}))>{}>>
    operator T() const { return detail::to_cpp<T>(handle_); }

    template <typename T,
              typename = std::enable_if_t<
                  std::is_same<T&, decltype(detail::to_cpp<T>(SCM{}))>{}>>
    operator T& () const { return detail::to_cpp<T>(handle_); }

    template <typename T,
              typename = std::enable_if_t<
                  std::is_same<const T&, decltype(detail::to_cpp<T>(SCM{}))>{}>>
    operator const T& () const { return detail::to_cpp<T>(handle_); }

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

#define SCM_DECLARE_WRAPPER_TYPE(cpp_name__)                            \
    namespace scm {                                                     \
    namespace detail {                                                  \
    template <>                                                         \
    struct convert<cpp_name__>                                          \
        : convert_wrapper_type<cpp_name__> {};                          \
    }} /* namespace scm::detail */                                      \
    /**/

SCM_DECLARE_WRAPPER_TYPE(val);
