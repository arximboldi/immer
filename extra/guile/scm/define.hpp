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

#include <scm/detail/invoke.hpp>
#include <scm/detail/function_args.hpp>
#include <scm/types.hpp>
#include <scm/list.hpp>

#include <string>
#include <type_traits>
#include <utility>

#include <boost/utility/typed_in_place_factory.hpp>

#include <iostream>

namespace scm {
namespace detail {

// This anonymous namespace should help avoiding registration clashes
// among translation units.
namespace {

template <typename... Ts>
void check_call_once()
{
    static bool called = false;
    if (called) scm_misc_error (nullptr, "Double defined binding. \
This may be caused because there are multiple C++ binding groups in the same \
translation unit.  You may solve this by using different type tags for each \
binding group.", SCM_EOL);
    called = true;
}

template <typename Tag, typename Fn>
auto subr_wrapper(Fn fn, pack<>)
{
    check_call_once<Tag, Fn>();
    static const Fn fn_ = fn;
    return [] () -> val { return to_scm(invoke(fn_)); };
}
template <typename Tag, typename Fn, typename T1>
auto subr_wrapper(Fn fn, pack<T1>)
{
    check_call_once<Tag, Fn>();
    static const Fn fn_ = fn;
    return [] (val a1) -> val {
        return to_scm(invoke(fn_,
                             to_cpp<T1>(a1)));
    };
}
template <typename Tag, typename Fn, typename T1, typename T2>
auto subr_wrapper(Fn fn, pack<T1, T2>)
{
    check_call_once<Tag, Fn>();
    static const Fn fn_ = fn;
    return [] (val a1, val a2) -> val {
        return to_scm(invoke(fn_,
                             to_cpp<T1>(a1),
                             to_cpp<T2>(a2)));
    };
}
template <typename Tag, typename Fn, typename T1, typename T2, typename T3>
auto subr_wrapper(Fn fn, pack<T1, T2, T3>)
{
    check_call_once<Tag, Fn>();
    static const Fn fn_ = fn;
    return [] (val a1, val a2, val a3) -> val {
        return to_scm(invoke(fn_,
                             to_cpp<T1>(a1),
                             to_cpp<T2>(a2),
                             to_cpp<T3>(a3)));
    };
}

} // anonymous namespace

template <typename Tag, typename Fn>
static void define_impl(const std::string& name, Fn fn)
{
    using args_t = function_args_t<Fn>;
    constexpr auto args_size = pack_size_v<args_t>;
    constexpr auto has_rest  = std::is_same<pack_last_t<args_t>, scm::args>{};
    constexpr auto arg_count = args_size - has_rest;
    auto subr = (scm_t_subr) +subr_wrapper<Tag, Fn>(fn, args_t{});
    scm_c_define_gsubr(name.c_str(), arg_count, 0, has_rest, subr);
    scm_c_export(name.c_str());
}

template <typename Tag, typename T, int Seq=0>
struct type_registrar
{
    using this_t = type_registrar;
    using next_t = type_registrar<Tag, T, Seq + 1>;

    std::string type_name;

    next_t constructor() &&
    {
        define_impl<this_t>(type_name, [] { return T{}; });
        return { std::move(type_name) };
    }

    template <typename Fn>
    next_t constructor(Fn fn) &&
    {
        define_impl<this_t>(type_name, fn);
        return { std::move(type_name) };
    }

    next_t maker() &&
    {
        define_impl<this_t>("make-" + type_name, [] { return T{}; });
        return { std::move(type_name) };
    }

    template <typename Fn>
    next_t maker(Fn fn) &&
    {
        define_impl<this_t>("make-" + type_name, fn);
        return { std::move(type_name) };
    }

    template <typename Fn>
    next_t define(std::string define_name, Fn fn) &&
    {
        define_impl<this_t>(type_name + "-" + define_name, fn);
        return { std::move(type_name) };
    }
};

} // namespace detail

template <typename Tag, typename T=Tag>
detail::type_registrar<Tag, T> type(std::string type_name)
{
    using storage_t = detail::foreign_type_storage<T>;
    assert(storage_t::data == SCM_UNSPECIFIED);
    storage_t::data = scm_make_foreign_object_type(
        scm_from_utf8_symbol(type_name.c_str()),
        scm_list_1(scm_from_utf8_symbol("data")),
        nullptr);
    return { type_name };
}

} // namespace scm
