//
// immer - immutable data structures for C++
// Copyright (C) 2016 Juan Pedro Bolivar Puente
//
// This file is part of immer.
//
// immer is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// immer is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with immer.  If not, see <http://www.gnu.org/licenses/>.
//

#include <immer/detail/ref_count_base.hpp>
#include <boost/intrusive_ptr.hpp>
#include <vector>

namespace immer {
namespace detail {

template <typename T>
struct array_data : std::vector<T>, ref_count_base<array_data<T>>
{
    using std::vector<T>::vector;

    array_data(const array_data& v) : std::vector<T> (v) {}
    array_data(array_data&& v) : std::vector<T> (std::move(v)) {}
};

template <typename T>
struct array_impl
{
    static const array_impl empty;

    const T& get(std::size_t index) const
    {
        return (*d)[index];
    }

    array_impl push_back(T value) const
    {
        return mutated([&] (auto& x) {
            x.push_back(std::move(value));
        });
    }

    array_impl assoc(std::size_t idx, T value) const
    {
        return mutated([&] (auto& x) {
            x[idx] = std::move(value);
        });
    }

    template <typename Fn>
    array_impl update(std::size_t idx, Fn&& op) const
    {
        return mutated([&] (auto& x) {
            x[idx] = std::forward<Fn>(op) (std::move(x[idx]));
        });
    }

    template <typename Fn>
    array_impl mutated(Fn&& op) const
    {
        auto x = new array_data<T>(*d);
        std::forward<Fn>(op) (*x);
        return { x };
    }

    boost::intrusive_ptr<array_data<T>> d;
};

template <typename T>
const array_impl<T> array_impl<T>::empty = { new array_data<T>{} };

} // namespace detail
} // namespace immer
