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

#include <immer/detail/rbts/rrbtree.hpp>
#include <immer/detail/iterator_facade.hpp>

namespace immer {
namespace detail {
namespace rbts {

template <typename T, typename MP, bits_t B, bits_t BL>
struct rrbtree_iterator
    : iterator_facade<rrbtree_iterator<T, MP, B, BL>,
                      std::random_access_iterator_tag,
                      T,
                      const T&>
{
    using tree_t   = rrbtree<T, MP, B, BL>;
    using region_t = std::tuple<const T*, size_t, size_t>;

    struct end_t {};

    const tree_t& impl() const { return *v_; }
    size_t index() const { return i_; }

    rrbtree_iterator() = default;

    rrbtree_iterator(const tree_t& v)
        : v_    { &v }
        , i_    { 0 }
        , curr_ { nullptr, ~size_t{}, ~size_t{} }
    {
    }

    rrbtree_iterator(const tree_t& v, end_t)
        : v_    { &v }
        , i_    { v.size }
        , curr_ { nullptr, ~size_t{}, ~size_t{} }
    {}

private:
    friend iterator_core_access;

    const tree_t* v_;
    size_t   i_;
    mutable region_t curr_;

    void increment()
    {
        using std::get;
        assert(i_ < v_->size);
        ++i_;
    }

    void decrement()
    {
        using std::get;
        assert(i_ > 0);
        --i_;
    }

    void advance(std::ptrdiff_t n)
    {
        using std::get;
        assert(n <= 0 || i_ + static_cast<size_t>(n) <= v_->size);
        assert(n >= 0 || static_cast<size_t>(-n) <= i_);
        i_ += n;
    }

    bool equal(const rrbtree_iterator& other) const
    {
        return i_ == other.i_;
    }

    std::ptrdiff_t distance_to(const rrbtree_iterator& other) const
    {
        return other.i_ > i_
            ?   static_cast<std::ptrdiff_t>(other.i_ - i_)
            : - static_cast<std::ptrdiff_t>(i_ - other.i_);
    }

    const T& dereference() const
    {
        using std::get;
        if (i_ < get<1>(curr_) || i_ >= get<2>(curr_))
            curr_ = v_->region_for(i_);
        return get<0>(curr_)[i_ - get<1>(curr_)];
    }
};

} // namespace rbts
} // namespace detail
} // namespace immer
