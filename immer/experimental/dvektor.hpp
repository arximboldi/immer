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

#include <immer/experimental/detail/dvektor_impl.hpp>
#include <immer/memory_policy.hpp>

namespace immer {

template <typename T,
          int B = 5,
          typename MemoryPolicy = default_memory_policy>
class dvektor
{
    using impl_t = detail::dvektor::impl<T, B, MemoryPolicy>;

public:
    using value_type = T;
    using reference = const T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using const_reference = const T&;

    using iterator         = detail::dvektor::iterator<T, B, MemoryPolicy>;
    using const_iterator   = iterator;
    using reverse_iterator = std::reverse_iterator<iterator>;

    dvektor() = default;

    iterator begin() const { return {impl_}; }
    iterator end()   const { return {impl_, typename iterator::end_t{}}; }

    reverse_iterator rbegin() const { return reverse_iterator{end()}; }
    reverse_iterator rend()   const { return reverse_iterator{begin()}; }

    std::size_t size() const { return impl_.size; }
    bool empty() const { return impl_.size == 0; }

    reference operator[] (size_type index) const
    { return impl_.get(index); }

    dvektor push_back(value_type value) const
    { return { impl_.push_back(std::move(value)) }; }

    dvektor assoc(std::size_t idx, value_type value) const
    { return { impl_.assoc(idx, std::move(value)) }; }

    template <typename FnT>
    dvektor update(std::size_t idx, FnT&& fn) const
    { return { impl_.update(idx, std::forward<FnT>(fn)) }; }

private:
    dvektor(impl_t impl) : impl_(std::move(impl)) {}
    impl_t impl_ = detail::dvektor::empty<T, B, MemoryPolicy>;
};

} // namespace immer
