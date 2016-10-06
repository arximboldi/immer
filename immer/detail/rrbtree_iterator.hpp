
#pragma once

#include <immer/detail/rrbtree.hpp>

#include <boost/iterator/iterator_facade.hpp>
#include <boost/iterator/iterator_facade.hpp>

namespace immer {
namespace detail {

template <typename T, int B, typename MP>
struct rrbtree_iterator : boost::iterator_facade<
    rrbtree_iterator<T, B, MP>,
    T,
    boost::random_access_traversal_tag,
    const T&>
{
    struct end_t {};

    rrbtree_iterator() = default;

    rrbtree_iterator(const rrbtree<T, B, MP>& v)
        : v_    { &v }
        , i_    { 0 }
        , base_ { 0 }
          //, curr_ { v.array_for(0) } // WIP
    {
    }

    rrbtree_iterator(const rrbtree<T, B, MP>& v, end_t)
        : v_    { &v }
        , i_    { v.size }
        , base_ { i_ - (i_ & mask<B>) }
          //, curr_ { v.array_for(i_ - 1) + (i_ - base_) } // WIP
    {}

private:
    friend class boost::iterator_core_access;

    const rrbtree<T, B, MP>* v_;
    std::size_t       i_;
    std::size_t       base_;
    const T*          curr_;

    void increment()
    {
        assert(i_ < v_->size);
        ++i_;
        if (i_ - base_ < branches<B>) {
            ++curr_;
        } else {
            base_ += branches<B>;
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
            base_ -= branches<B>;
            curr_ = v_->array_for(i_) + (branches<B> - 1);
        }
    }

    void advance(std::ptrdiff_t n)
    {
        assert(n <= 0 || i_ + static_cast<std::size_t>(n) <= v_->size);
        assert(n >= 0 || static_cast<std::size_t>(-n) <= i_);

        i_ += n;
        if (i_ <= base_ && i_ - base_ < branches<B>) {
            curr_ += n;
        } else {
            base_ = i_ - (i_ & mask<B>);
            curr_ = v_->array_for(i_) + (i_ - base_);
        }
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
        return *curr_;
    }
};

} // namespace detail
} // namespace immer
