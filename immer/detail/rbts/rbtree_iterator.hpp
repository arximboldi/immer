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

#include <immer/detail/rbts/rbtree.hpp>

#include <boost/iterator/iterator_facade.hpp>
#include <boost/iterator/iterator_facade.hpp>

namespace immer {
namespace detail {
namespace rbts {

template <typename T, typename MP, bits_t B, bits_t BL>
struct rbtree_iterator : boost::iterator_facade<
    rbtree_iterator<T, MP, B, BL>,
    T,
    boost::random_access_traversal_tag,
    const T&>
{
    using tree_t = rbtree<T, MP, B, BL>;

    struct end_t {};

    rbtree_iterator() = default;

    rbtree_iterator(const tree_t& v)
        : v_    { &v }
        , i_    { 0 }
        , base_ { 0 }
        , curr_ { v.array_for(0) }
    {
    }

    rbtree_iterator(const tree_t& v, end_t)
        : v_    { &v }
        , i_    { v.size }
        , base_ { i_ - (i_ & mask<BL>) }
        , curr_ { v.array_for(i_ - 1) + (i_ - base_) }
    {}

private:
    friend class boost::iterator_core_access;

    const tree_t* v_;
    size_t    i_;
    size_t    base_;
    const T*  curr_;

    void increment()
    {
        assert(i_ < v_->size);
        ++i_;
        if (i_ - base_ < branches<BL>) {
            ++curr_;
        } else {
            base_ += branches<BL>;
            curr_ = v_->array_for(i_);
        }
    }

    void decrement()
    {
        assert(i_ > 0);
        --i_;
        if (i_ >= base_) {
            --curr_;
        } else {
            base_ -= branches<BL>;
            curr_ = v_->array_for(i_) + (branches<BL> - 1);
        }
    }

    void advance(std::ptrdiff_t n)
    {
        assert(n <= 0 || i_ + static_cast<size_t>(n) <= v_->size);
        assert(n >= 0 || static_cast<size_t>(-n) <= i_);

        i_ += n;
        if (i_ <= base_ && i_ - base_ < branches<BL>) {
            curr_ += n;
        } else {
            base_ = i_ - (i_ & mask<BL>);
            curr_ = v_->array_for(i_) + (i_ - base_);
        }
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
        return *curr_;
    }
};

} // namespace rbts
} // namespace detail
} // namespace immer
