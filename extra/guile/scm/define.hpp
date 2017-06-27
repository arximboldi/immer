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

#include <scm/detail/subr_wrapper.hpp>
#include <scm/detail/finalizer_wrapper.hpp>
#include <scm/val.hpp>
#include <scm/list.hpp>

#include <string>
#include <type_traits>
#include <utility>

#include <boost/utility/typed_in_place_factory.hpp>

#include <iostream>

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
    scm_c_export(name.c_str());
}

template <typename T>
struct foreign_type_storage
{
    static SCM data;
};

template <typename T>
SCM foreign_type_storage<T>::data = SCM_UNSPECIFIED;

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

struct move_sequence
{
    move_sequence() = default;
    move_sequence(const move_sequence&) = delete;
    move_sequence(move_sequence&& other)
    { other.moved_from_ = true; };

    bool moved_from_ = false;
};

template <typename Tag, typename T, int Seq=0>
struct type_registrar : move_sequence
{
    using this_t = type_registrar;
    using next_t = type_registrar<Tag, T, Seq + 1>;

    std::string type_name_ = {};
    scm_t_struct_finalize finalizer_ = nullptr;

    type_registrar(type_registrar&&) = default;

    type_registrar(std::string type_name)
        : type_name_(std::move(type_name))
    {}

    ~type_registrar()
    {
        if (!moved_from_) {
            using storage_t = detail::foreign_type_storage<T>;
            assert(storage_t::data == SCM_UNSPECIFIED);
            storage_t::data = scm_make_foreign_object_type(
                scm_from_utf8_symbol(type_name_.c_str()),
                scm_list_1(scm_from_utf8_symbol("data")),
                finalizer_);
        }
    }

    template <int Seq2, typename Enable=std::enable_if_t<Seq2 + 1 == Seq>>
    type_registrar(type_registrar<Tag, T, Seq2> r)
        : type_name_{std::move(r.type_name_)}
        , finalizer_{std::move(r.finalizer_)}
    {}

    next_t constructor() &&
    {
        define_impl<this_t>(type_name_, [] { return T{}; });
        return { std::move(*this) };
    }

    template <typename Fn>
    next_t constructor(Fn fn) &&
    {
        define_impl<this_t>(type_name_, fn);
        return { std::move(*this) };
    }

    next_t finalizer() &&
    {
        finalizer_ = (scm_t_struct_finalize) +finalizer_wrapper<Tag>(
            [] (T& x) { x.~T(); });
        return { std::move(*this) };
    }

    template <typename Fn>
    next_t finalizer(Fn fn) &&
    {
        finalizer_ = (scm_t_struct_finalize) +finalizer_wrapper<Tag>(fn);
        return { std::move(*this) };
    }

    next_t maker() &&
    {
        define_impl<this_t>("make-" + type_name_, [] { return T{}; });
        return { std::move(*this) };
    }

    template <typename Fn>
    next_t maker(Fn fn) &&
    {
        define_impl<this_t>("make-" + type_name_, fn);
        return { std::move(*this) };
    }

    template <typename Fn>
    next_t define(std::string define_name, Fn fn) &&
    {
        define_impl<this_t>(type_name_ + "-" + define_name, fn);
        return { std::move(*this) };
    }
};

} // namespace detail

template <typename Tag, typename T=Tag>
detail::type_registrar<Tag, T> type(std::string type_name)
{
    return { type_name };
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
