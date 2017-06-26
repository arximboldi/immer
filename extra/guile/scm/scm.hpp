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
#include <scm/args.hpp>

#include <string>
#include <type_traits>
#include <utility>

#include <boost/optional.hpp>
#include <boost/utility/typed_in_place_factory.hpp>

#include <iostream>

namespace scm {
namespace detail {

template <typename Id, typename Fn>
struct subr_storage
{
    static boost::optional<Fn> data;
};

template <typename Id, typename Fn>
boost::optional<Fn> subr_storage<Id, Fn>::data = boost::none;

template <typename Storage>
auto subr_invoker_aux(pack<>)
{
    return [] () -> SCM { return to_scm(invoke(*Storage::data)); };
}
template <typename Storage, typename T1>
auto subr_invoker_aux(pack<T1>)
{
    return [] (SCM a1) -> SCM {
        return to_scm(invoke(*Storage::data,
                             to_cpp<T1>(a1)));
    };
}
template <typename Storage, typename T1, typename T2>
auto subr_invoker_aux(pack<T1, T2>)
{
    return [] (SCM a1, SCM a2) -> SCM {
        return to_scm(invoke(*Storage::data,
                             to_cpp<T1>(a1),
                             to_cpp<T2>(a2)));
    };
}
template <typename Storage, typename T1, typename T2, typename T3>
auto subr_invoker_aux(pack<T1, T2, T3>)
{
    return [] (SCM a1, SCM a2, SCM a3) -> SCM {
        return to_scm(invoke(*Storage::data,
                             to_cpp<T1>(a1),
                             to_cpp<T2>(a2),
                             to_cpp<T3>(a3)));
    };
}

template <typename TypeT, int Seq>
struct subr_id
{
    template <typename Fn>
    static void store(Fn fn)
    {
        using storage_t = subr_storage<subr_id, Fn>;
        assert(!storage_t::data);
        storage_t::data = boost::in_place<Fn>(fn);
    }

    template <typename Fn>
    static auto invoker()
    {
        using storage_t = subr_storage<subr_id, Fn>;
        using args_t = function_args_t<Fn>;
        return subr_invoker_aux<storage_t>(args_t{});
    }

    template <typename Fn>
    static void define(const std::string& name, Fn fn)
    {
        store(fn);
        using args_t = function_args_t<Fn>;
        constexpr auto args_size = pack_size_v<args_t>;
        constexpr auto has_rest  = std::is_same<pack_last_t<args_t>,
                                                scm::args>{};
        constexpr auto arg_count = args_size - has_rest;
        auto subr = (scm_t_subr) +invoker<Fn>();
        scm_c_define_gsubr(name.c_str(), arg_count, 0, has_rest, subr);
        scm_c_export(name.c_str());
    }
};

template <typename Tag, typename T, int Seq=0>
struct type_registrar
{
    using next_t    = type_registrar<Tag, T, Seq + 1>;
    using subr_id_t = subr_id<Tag, Seq>;

    std::string type_name;

    next_t constructor() &&
    {
        subr_id_t::define(type_name, [] { return T{}; });
        return { std::move(type_name) };
    }

    template <typename Fn>
    next_t constructor(Fn fn) &&
    {
        subr_id_t::define(type_name, fn);
        return { std::move(type_name) };
    }

    next_t maker() &&
    {
        subr_id_t::define("make-" + type_name, [] { return T{}; });
        return { std::move(type_name) };
    }

    template <typename Fn>
    next_t maker(Fn fn) &&
    {
        subr_id_t::define("make-" + type_name, fn);
        return { std::move(type_name) };
    }

    template <typename Fn>
    next_t define(std::string define_name, Fn fn) &&
    {
        subr_id_t::define(type_name + "-" + define_name, fn);
        return { std::move(type_name) };
    }
};

} // namespace detail

template <typename Tag, typename T=Tag>
detail::type_registrar<Tag, T> type(std::string type_name)
{
    using meta_t = detail::type_meta<T>;
    assert(!meta_t::foreign_type);
    meta_t::foreign_type = scm_make_foreign_object_type(
        scm_from_utf8_symbol(type_name.c_str()),
        scm_list_1(scm_from_utf8_symbol("data")),
        nullptr);
    return { type_name };
}

} // namespace scm

#define SCM_INIT(name__)                                \
    extern "C" void init_ ## name__()                   \
    /**/
