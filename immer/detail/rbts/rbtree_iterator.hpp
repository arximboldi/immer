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

#include <immer/detail/rbts/rbtree.hpp>
#include <immer/detail/iterator_facade.hpp>

namespace immer {
namespace detail {
namespace rbts {

template <typename T, typename MP, bits_t B, bits_t BL>
struct rbtree_iterator
    : iterator_facade<rbtree_iterator<T, MP, B, BL>,
                      std::random_access_iterator_tag,
                      T,
                      const T&>
{
    using tree_t = rbtree<T, MP, B, BL>;

    struct end_t {};

    rbtree_iterator() = default;

    rbtree_iterator(const tree_t& v)
        : v_    { &v }
        , i_    { 0 }
        , base_ { ~size_t{} }
        , curr_ { nullptr }
    {}

    rbtree_iterator(const tree_t& v, end_t)
        : v_    { &v }
        , i_    { v.size }
        , base_ { ~size_t{} }
        , curr_ { nullptr }
    {}

    const tree_t& impl() const { return *v_; }
    size_t index() const { return i_; }

private:
    friend iterator_core_access;

    const tree_t*    v_;
    size_t           i_;
    mutable size_t   base_;
    mutable const T* curr_ = nullptr;

    void increment()
    {
        assert(i_ < v_->size);
        ++i_;
    }

    void decrement()
    {
        assert(i_ > 0);
        --i_;
    }

    void advance(std::ptrdiff_t n)
    {
        assert(n <= 0 || i_ + static_cast<size_t>(n) <= v_->size);
        assert(n >= 0 || static_cast<size_t>(-n) <= i_);
        i_ += n;
    }

    bool equal(const rbtree_iterator& other) const
    {
        return i_ == other.i_;
    }

    std::ptrdiff_t distance_to(const rbtree_iterator& other) const
    {
        return other.i_ > i_
            ?   static_cast<std::ptrdiff_t>(other.i_ - i_)
            : - static_cast<std::ptrdiff_t>(i_ - other.i_);
    }

    const T& dereference() const
    {
        auto base = i_ & ~mask<BL>;
        if (base_ != base) {
            base_ = base;
            curr_ = v_->array_for(i_);
        }
        return curr_[i_ & mask<BL>];
    }
};

} // namespace rbts
} // namespace detail
} // namespace immer
