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

#pragma once

#include <immer/detail/array_impl.hpp>

namespace immer {

template <typename T>
class array
{
    using impl_t = detail::array_impl<T>;

public:
    using value_type = T;
    using reference = const T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using const_reference = const T&;

    using iterator         = typename std::vector<T>::iterator;
    using const_iterator   = iterator;
    using reverse_iterator = typename std::vector<T>::reverse_iterator;

    array() = default;

    iterator begin() const { return impl_.d->begin(); }
    iterator end()   const { return impl_.d->end(); }

    reverse_iterator rbegin() const { return impl_.d->rbegin(); }
    reverse_iterator rend()   const { return impl_.d->rend(); }

    std::size_t size() const { return impl_.d->size(); }
    bool empty() const { return impl_.d->empty(); }

    reference operator[] (size_type index) const
    { return impl_.get(index); }

    array push_back(value_type value) const
    { return { impl_.push_back(std::move(value)) }; }

    array assoc(std::size_t idx, value_type value) const
    { return { impl_.assoc(idx, std::move(value)) }; }

    template <typename FnT>
    array update(std::size_t idx, FnT&& fn) const
    { return { impl_.update(idx, std::forward<FnT>(fn)) }; }

private:
    array(impl_t impl) : impl_(std::move(impl)) {}
    impl_t impl_ = impl_t::empty;
};

} /* namespace immer */
