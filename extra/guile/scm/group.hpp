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

#include <scm/detail/define.hpp>
#include <string>

namespace scm {
namespace detail {

template <typename Tag, int Seq=0>
struct definer
{
    using this_t = definer;
    using next_t = definer<Tag, Seq + 1>;

    std::string group_name_ = {};

    definer() = default;
    definer(definer&&) = default;

    template <int Seq2,
              typename Enable=std::enable_if_t<Seq2 + 1 == Seq>>
    definer(definer<Tag, Seq2>)
    {}

    template <typename Fn>
    next_t define(std::string name, Fn fn) &&
    {
        define_impl<this_t>(name, fn);
        return { std::move(*this) };
    }

    template <typename Fn>
    next_t maker(Fn fn) &&
    {
        define_impl<this_t>("make", fn);
        return { std::move(*this) };
    }
};

template <typename Tag, int Seq=0>
struct group_definer
{
    using this_t = group_definer;
    using next_t = group_definer<Tag, Seq + 1>;

    std::string group_name_ = {};

    group_definer(std::string name)
        : group_name_{std::move(name)} {}

    group_definer(group_definer&&) = default;

    template <int Seq2,
              typename Enable=std::enable_if_t<Seq2 + 1 == Seq>>
    group_definer(group_definer<Tag, Seq2>)
    {}

    template <typename Fn>
    next_t define(std::string name, Fn fn) &&
    {
        define_impl<this_t>(group_name_ + "-" + name, fn);
        return { std::move(*this) };
    }
};

} // namespace detail

template <typename Tag=void>
detail::definer<Tag> group()
{
    return {};
}

template <typename Tag=void>
detail::group_definer<Tag> group(std::string name)
{
    return { std::move(name) };
}

} // namespace scm
