//
// immer - immutable data structures for C++
// Copyright (C) 2016, 2017 Juan Pedro Bolivar Puente
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

#include <immer/memory_policy.hpp>
#include <immer/detail/hamts/champ.hpp>

#include <functional>

namespace immer {

template <typename K,
          typename T,
          typename Hash          = std::hash<T>,
          typename Equal         = std::equal_to<T>,
          typename MemoryPolicy  = default_memory_policy,
          detail::hamts::bits_t B = default_bits>
class map
{
    using value_t = std::pair<const K, T>;

    struct hash_key
    {
        auto operator() (const value_t& v)
        { return Hash{}(v.first); }

        auto operator() (const K& v)
        { return Hash{}(v); }
    };

    struct equal_key
    {
        auto operator() (const value_t& a, const value_t& b)
        { return Equal{}(a.first, b.first); }

        auto operator() (const value_t& a, const K& b)
        { return Equal{}(a.first, b); }
    };

    using impl_t = detail::hamts::champ<
        value_t, hash_key, equal_key, MemoryPolicy, B>;

public:
    using key_type = K;
    using mapped_type = T;
    using value_type = std::pair<const K, T>;
    using size_type = detail::hamts::size_t;
    using diference_type = std::ptrdiff_t;
    using hasher = Hash;
    using key_equal = Equal;
    using reference = const value_type&;
    using const_reference = const value_type&;

    map() = default;

    /*!
     * Returns the number of elements in the container.  It does
     * not allocate memory and its complexity is @f$ O(1) @f$.
     */
    size_type size() const { return impl_.size; }

    map insert(value_type v) const
    { return impl_.add(std::move(v)); }

    size_type count(const K& k) const
    { return impl_.get(k) ? 1 : 0; }

private:
    map(impl_t impl)
        : impl_(std::move(impl))
    {}

    impl_t impl_ = impl_t::empty;
};

} // namespace immer
