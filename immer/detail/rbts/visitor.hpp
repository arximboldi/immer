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

#include <tuple>
#include <utility>

namespace immer {
namespace detail {
namespace rbts {

struct visitor_tag {};

template <typename... Fns>
using fn_visitor = std::tuple<visitor_tag, Fns...>;

template <typename... Fns>
auto make_visitor(Fns&& ...fns)
{
    return std::make_tuple(visitor_tag{}, std::forward<Fns>(fns)...);
}

template <typename FnR, typename FnI, typename FnL, typename... Args>
decltype(auto) visit_relaxed(fn_visitor<FnR, FnI, FnL> v, Args&& ...args)
{ return std::get<1>(v)(v, std::forward<Args>(args)...); }

template <typename FnR, typename FnI, typename FnL, typename... Args>
decltype(auto) visit_regular(fn_visitor<FnR, FnI, FnL> v, Args&& ...args)
{ return std::get<2>(v)(v, std::forward<Args>(args)...); }

template <typename FnR, typename FnI, typename FnL, typename... Args>
decltype(auto) visit_leaf(fn_visitor<FnR, FnI, FnL> v, Args&& ...args)
{ return std::get<3>(v)(v, std::forward<Args>(args)...); }

template <typename FnI, typename FnL, typename... Args>
decltype(auto) visit_inner(fn_visitor<FnI, FnL> v, Args&& ...args)
{ return std::get<1>(v)(v, std::forward<Args>(args)...); }

template <typename FnI, typename FnL, typename... Args>
decltype(auto) visit_leaf(fn_visitor<FnI, FnL> v, Args&& ...args)
{ return std::get<2>(v)(v, std::forward<Args>(args)...); }

template <typename Fn, typename... Args>
decltype(auto) visit_node(fn_visitor<Fn> v, Args&& ...args)
{ return std::get<1>(v)(v, std::forward<Args>(args)...); }

template <typename Visitor, typename... Args>
decltype(auto) visit_relaxed(Visitor&& v, Args&& ...args)
{
    return visit_inner(std::forward<Visitor>(v),
                       std::forward<Args>(args)...);
}

template <typename Visitor, typename... Args>
decltype(auto) visit_regular(Visitor&& v, Args&& ...args)
{
    return visit_inner(std::forward<Visitor>(v),
                       std::forward<Args>(args)...);
}

template <typename Visitor, typename... Args>
decltype(auto) visit_inner(Visitor&& v, Args&& ...args)
{
    return visit_node(std::forward<Visitor>(v),
                      std::forward<Args>(args)...);
}

template <typename Visitor, typename... Args>
decltype(auto) visit_leaf(Visitor&& v, Args&& ...args)
{
    return visit_node(std::forward<Visitor>(v),
                      std::forward<Args>(args)...);
}

} // namespace rbts
} // namespace detail
} // namespace immer
