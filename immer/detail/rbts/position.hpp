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

#include <immer/config.hpp>
#include <immer/detail/rbts/bits.hpp>

#include <cassert>
#include <utility>
#include <type_traits>

namespace immer {
namespace detail {
namespace rbts {

template <typename Pos>
constexpr auto bits = std::decay_t<Pos>::node_t::bits;

template <typename Pos>
constexpr auto bits_leaf = std::decay_t<Pos>::node_t::bits_leaf;

template <typename Pos>
using node_type = typename std::decay<Pos>::type::node_t;

template <typename NodeT>
struct empty_regular_pos
{
    using node_t = NodeT;
    node_t* node_;

    count_t count() const { return 0; }
    node_t* node()  const { return node_; }
    shift_t shift() const { return 0; }
    size_t  size()  const { return 0; }

    template <typename Visitor, typename... Args>
    void each(Visitor, Args&&...) {}

    template <typename Visitor, typename... Args>
    decltype(auto) visit(Visitor v, Args&& ...args)
    {
        return visit_regular(v, *this, std::forward<Args>(args)...);
    }
};

template <typename NodeT>
empty_regular_pos<NodeT> make_empty_regular_pos(NodeT* node)
{
    return {node};
}

template <typename NodeT>
struct empty_leaf_pos
{
    using node_t = NodeT;
    node_t* node_;

    count_t count() const { return 0; }
    node_t* node()  const { return node_; }
    shift_t shift() const { return 0; }
    size_t  size()  const { return 0; }

    template <typename Visitor, typename ...Args>
    decltype(auto) visit(Visitor v, Args&& ...args)
    {
        return visit_leaf(v, *this, std::forward<Args>(args)...);
    }
};

template <typename NodeT>
empty_leaf_pos<NodeT> make_empty_leaf_pos(NodeT* node)
{
    assert(node);
    return {node};
}

template <typename NodeT>
struct leaf_pos
{
    static constexpr auto B  = NodeT::bits;
    static constexpr auto BL = NodeT::bits_leaf;

    using node_t = NodeT;
    node_t* node_;
    size_t size_;

    count_t count() const { return index(size_ - 1) + 1; }
    node_t* node()  const { return node_; }
    size_t  size()  const { return size_; }
    shift_t shift() const { return 0; }
    count_t index(size_t idx) const { return idx & mask<BL>; }
    count_t subindex(size_t idx) const { return idx; }

    template <typename Visitor, typename ...Args>
    decltype(auto) visit(Visitor v, Args&& ...args)
    {
        return visit_leaf(v, *this, std::forward<Args>(args)...);
    }
};

template <typename NodeT>
leaf_pos<NodeT> make_leaf_pos(NodeT* node, size_t size)
{
    assert(node);
    assert(size > 0);
    return {node, size};
}

template <typename NodeT>
struct leaf_sub_pos
{
    static constexpr auto B  = NodeT::bits;
    static constexpr auto BL = NodeT::bits_leaf;

    using node_t = NodeT;
    node_t* node_;
    count_t count_;

    count_t count() const { return count_; }
    node_t* node()  const { return node_; }
    size_t  size()  const { return count_; }
    shift_t shift() const { return 0; }
    count_t index(size_t idx) const { return idx & mask<BL>; }
    count_t subindex(size_t idx) const { return idx; }

    template <typename Visitor, typename ...Args>
    decltype(auto) visit(Visitor v, Args&& ...args)
    {
        return visit_leaf(v, *this, std::forward<Args>(args)...);
    }
};

template <typename NodeT>
leaf_sub_pos<NodeT> make_leaf_sub_pos(NodeT* node, count_t count)
{
    assert(node);
    assert(count > 0);
    assert(count <= branches<NodeT::bits_leaf>);
    return {node, count};
}

template <typename NodeT>
struct full_leaf_pos
{
    static constexpr auto B  = NodeT::bits;
    static constexpr auto BL = NodeT::bits_leaf;

    using node_t = NodeT;
    node_t* node_;

    count_t count() const { return branches<BL>; }
    node_t* node()  const { return node_; }
    size_t  size()  const { return branches<BL>; }
    shift_t shift() const { return 0; }
    count_t index(size_t idx) const { return idx & mask<BL>; }
    count_t subindex(size_t idx) const { return idx; }

    template <typename Visitor, typename ...Args>
    decltype(auto) visit(Visitor v, Args&& ...args)
    {
        return visit_leaf(v, *this, std::forward<Args>(args)...);
    }
};

template <typename NodeT>
full_leaf_pos<NodeT> make_full_leaf_pos(NodeT* node)
{
    assert(node);
    return {node};
}

template <typename NodeT>
struct regular_pos
{
    static constexpr auto B  = NodeT::bits;
    static constexpr auto BL = NodeT::bits_leaf;

    using node_t = NodeT;
    node_t* node_;
    shift_t shift_;
    size_t size_;

    count_t count() const { return index(size_ - 1) + 1; }
    node_t* node()  const { return node_; }
    size_t  size()  const { return size_; }
    shift_t shift() const { return shift_; }
    count_t index(size_t idx) const { return (idx >> shift_) & mask<B>; }
    count_t subindex(size_t idx) const { return idx >> shift_; }
    size_t  this_size() const { return ((size_ - 1) & ~(~size_t{} << (shift_ + B))) + 1; }

    template <typename Visitor, typename... Args>
    void each(Visitor v, Args&&... args)
    { return each_regular(*this, v, args...); }

    template <typename Visitor, typename... Args>
    decltype(auto) towards(Visitor v, size_t idx, Args&&... args)
    { return towards_oh_ch_regular(*this, v, idx, index(idx), count(), args...); }

    template <typename Visitor, typename... Args>
    decltype(auto) towards_oh(Visitor v, size_t idx,
                              count_t offset_hint,
                              Args&&... args)
    { return towards_oh_ch_regular(*this, v, idx, offset_hint, count(), args...); }

    template <typename Visitor, typename... Args>
    decltype(auto) towards_oh_ch(Visitor v, size_t idx,
                                 count_t offset_hint,
                                 count_t count_hint,
                                 Args&&... args)
    { return towards_oh_ch_regular(*this, v, idx, offset_hint, count(), args...); }

    template <typename Visitor, typename... Args>
    decltype(auto) towards_sub_oh(Visitor v, size_t idx,
                                  count_t offset_hint,
                                  Args&&... args)
    { return towards_sub_oh_regular(*this, v, idx, offset_hint, args...); }

    template <typename Visitor, typename... Args>
    decltype(auto) last_oh(Visitor v, count_t offset_hint, Args&&... args)
    { return last_oh_regular(*this, v, offset_hint, args...); }

    template <typename Visitor, typename ...Args>
    decltype(auto) visit(Visitor v, Args&& ...args)
    {
        return visit_regular(v, *this, std::forward<Args>(args)...);
    }
};

template <typename Pos, typename Visitor, typename... Args>
void each_regular(Pos&& p, Visitor v, Args&&... args)
{
    constexpr auto B  = bits<Pos>;
    constexpr auto BL = bits_leaf<Pos>;

    if (p.shift() == BL) {
        auto n = p.node()->inner();
        auto last = p.count() - 1;
        auto e = n + last;
        for (; n != e; ++n)
            make_full_leaf_pos(*n).visit(v, args...);
        make_leaf_pos(*n, p.size()).visit(v, args...);
    } else {
        auto n = p.node()->inner();
        auto last = p.count() - 1;
        auto e = n + last;
        auto ss = p.shift() - B;
        for (; n != e; ++n)
            make_full_pos(*n, ss).visit(v, args...);
        make_regular_pos(*n, ss, p.size()).visit(v, args...);
    }
}

template <typename Pos, typename Visitor, typename... Args>
decltype(auto) towards_oh_ch_regular(Pos&& p, Visitor v, size_t idx,
                                     count_t offset_hint,
                                     count_t count_hint,
                                     Args&&... args)
{
    constexpr auto B  = bits<Pos>;
    constexpr auto BL = bits_leaf<Pos>;
    assert(offset_hint == p.index(idx));
    assert(count_hint  == p.count());
    auto is_leaf = p.shift() == BL;
    auto child   = p.node()->inner() [offset_hint];
    auto is_full = offset_hint + 1 != count_hint;
    return is_full
        ? (is_leaf
           ? make_full_leaf_pos(child).visit(v, idx, args...)
           : make_full_pos(child, p.shift() - B).visit(v, idx, args...))
        : (is_leaf
           ? make_leaf_pos(child, p.size()).visit(v, idx, args...)
           : make_regular_pos(child, p.shift() - B, p.size()).visit(v, idx, args...));
}

template <typename Pos, typename Visitor, typename... Args>
decltype(auto) towards_sub_oh_regular(Pos&& p, Visitor v, size_t idx,
                                      count_t offset_hint,
                                      Args&&... args)
{
    constexpr auto B  = bits<Pos>;
    constexpr auto BL = bits_leaf<Pos>;
    assert(offset_hint == p.index(idx));
    auto is_leaf = p.shift() == BL;
    auto child   = p.node()->inner() [offset_hint];
    auto lsize   = offset_hint << p.shift();
    auto size    = p.this_size();
    auto is_full = (size - lsize) >= (1u << p.shift());
    return is_full
        ? (is_leaf
           ? make_full_leaf_pos(child).visit(
               v, idx - lsize, args...)
           : make_full_pos(child, p.shift() - B).visit(
               v, idx - lsize, args...))
        : (is_leaf
           ? make_leaf_sub_pos(child, size - lsize).visit(
               v, idx - lsize, args...)
           : make_regular_sub_pos(child, p.shift() - B, size - lsize).visit(
               v, idx - lsize, args...));
}

template <typename Pos, typename Visitor, typename... Args>
decltype(auto) last_oh_regular(Pos&& p, Visitor v,
                               count_t offset_hint,
                               Args&&... args)
{
    assert(offset_hint == p.count() - 1);
    constexpr auto B  = bits<Pos>;
    constexpr auto BL = bits_leaf<Pos>;
    auto child     = p.node()->inner() [offset_hint];
    auto is_leaf   = p.shift() == BL;
    return is_leaf
        ? make_leaf_pos(child, p.size()).visit(v, args...)
        : make_regular_pos(child, p.shift() - B, p.size()).visit(v, args...);
}

template <typename NodeT>
regular_pos<NodeT> make_regular_pos(NodeT* node,
                                    shift_t shift,
                                    size_t size)
{
    assert(node);
    assert(shift >= NodeT::bits_leaf);
    assert(size > 0);
    return {node, shift, size};
}

struct null_sub_pos
{
    auto node() const { return nullptr; }

    template <typename Visitor, typename... Args>
    void each_sub(Visitor, Args&&...) {}
    template <typename Visitor, typename... Args>
    void each_right_sub(Visitor, Args&&...) {}
    template <typename Visitor, typename... Args>
    void each_left_sub(Visitor, Args&&...) {}
};

template <typename NodeT>
struct regular_sub_pos
{
    static constexpr auto B  = NodeT::bits;
    static constexpr auto BL = NodeT::bits_leaf;

    using node_t = NodeT;
    node_t* node_;
    shift_t shift_;
    size_t size_;

    count_t count() const { return subindex(size_ - 1) + 1; }
    node_t* node()  const { return node_; }
    size_t  size()  const { return size_; }
    shift_t shift() const { return shift_; }
    count_t index(size_t idx) const { return (idx >> shift_) & mask<B>; }
    count_t subindex(size_t idx) const { return idx >> shift_; }
    size_t  size_before(count_t offset) const { return offset << shift_; }
    size_t  this_size() const { return size_; }

    auto size(count_t offset)
    {
        return offset == subindex(size_ - 1)
            ? size_ - size_before(offset)
            : 1 << shift_;
    }

    auto size_sbh(count_t offset, size_t size_before_hint)
    {
        assert(size_before_hint == size_before(offset));
        return offset == subindex(size_ - 1)
            ? size_ - size_before_hint
            : 1 << shift_;
    }

    void copy_sizes(count_t offset,
                    count_t n,
                    size_t init,
                    size_t* sizes)
    {
        if (n) {
            auto last = offset + n - 1;
            auto e = sizes + n - 1;
            for (; sizes != e; ++sizes)
                init = *sizes = init + (1 << shift_);
            *sizes = init + size(last);
        }
    }

    template <typename Visitor, typename... Args>
    void each(Visitor v, Args&& ...args)
    { return each_regular(*this, v, args...); }

    template <typename Visitor, typename... Args>
    void each_sub_(Visitor v, count_t i, Args&& ...args)
    {
        auto last  = count() - 1;
        auto lsize = size_ - (last << shift_);
        auto n = node()->inner() + i;
        auto e = node()->inner() + last;
        if (shift() == BL) {
            for (; n != e; ++n)
                make_full_leaf_pos(*n).visit(v, args...);
            make_leaf_sub_pos(*n, lsize).visit(v, args...);
        } else {
            auto ss = shift_ - B;
            for (; n != e; ++n)
                make_full_pos(*n, ss).visit(v, args...);
            make_regular_sub_pos(*n, ss, lsize).visit(v, args...);
        }
    }

    template <typename Visitor, typename... Args>
    void each_sub(Visitor v, Args&& ...args)
    { each_sub_(v, 0, args...); }

    template <typename Visitor, typename... Args>
    void each_right_sub(Visitor v, Args&& ...args)
    { if (count() > 1) each_sub_(v, 1, args...); }

    template <typename Visitor, typename... Args>
    void each_left_sub(Visitor v, Args&& ...args)
    {
        auto last  = count() - 1;
        auto n = node_->inner();
        auto e = n + last;
        if (shift_ == BL) {
            for (; n != e; ++n)
                make_full_leaf_pos(*n).visit(v, args...);
        } else {
            auto ss = shift_ - B;
            for (; n != e; ++n)
                make_full_pos(*n, ss).visit(v, args...);
        }
    }

    template <typename Visitor, typename... Args>
    decltype(auto) towards(Visitor v, size_t idx, Args&&... args)
    { return towards_oh_ch_regular(*this, v, idx, index(idx), count(), args...); }

    template <typename Visitor, typename... Args>
    decltype(auto) towards_oh(Visitor v, size_t idx,
                              count_t offset_hint,
                              Args&&... args)
    { return towards_oh_ch_regular(*this, v, idx, offset_hint, count(), args...); }

    template <typename Visitor, typename... Args>
    decltype(auto) towards_oh_ch(Visitor v, size_t idx,
                                 count_t offset_hint,
                                 count_t count_hint,
                                 Args&&... args)
    { return towards_oh_ch_regular(*this, v, idx, offset_hint, count(), args...); }

    template <typename Visitor, typename... Args>
    decltype(auto) towards_sub_oh(Visitor v, size_t idx,
                                  count_t offset_hint,
                                  Args&& ...args)
    { return towards_sub_oh_regular(*this, v, idx, offset_hint, args...); }

    template <typename Visitor, typename... Args>
    decltype(auto) last_oh(Visitor v, count_t offset_hint, Args&&... args)
    { return last_oh_regular(*this, v, offset_hint, args...); }

    template <typename Visitor, typename... Args>
    decltype(auto) last_sub(Visitor v, Args&&... args)
    {
        auto offset    = count() - 1;
        auto child     = node_->inner() [offset];
        auto is_leaf   = shift_ == BL;
        auto lsize     = size_ - (offset << shift_);
        return is_leaf
            ? make_leaf_sub_pos(child, lsize).visit(v, args...)
            : make_regular_sub_pos(child, shift_ - B, lsize).visit(v, args...);
    }

    template <typename Visitor, typename... Args>
    decltype(auto) first_sub(Visitor v, Args&&... args)
    {
        auto is_leaf = shift_ == BL;
        auto child   = node_->inner() [0];
        auto is_full = size_ >= (1u << shift_);
        return is_full
            ? (is_leaf
               ? make_full_leaf_pos(child).visit(v, args...)
               : make_full_pos(child, shift_ - B).visit(v, args...))
            : (is_leaf
               ? make_leaf_sub_pos(child, size_).visit(v, args...)
               : make_regular_sub_pos(child, shift_ - B, size_).visit(v, args...));
    }

    template <typename Visitor, typename... Args>
    decltype(auto) first_sub_leaf(Visitor v, Args&&... args)
    {
        assert(shift_ == BL);
        auto child   = node_->inner() [0];
        auto is_full = size_ >= branches<BL>;
        return is_full
            ? make_full_leaf_pos(child).visit(v, args...)
            : make_leaf_sub_pos(child, size_).visit(v, args...);
    }

    template <typename Visitor, typename ...Args>
    decltype(auto) visit(Visitor v, Args&& ...args)
    {
        return visit_regular(v, *this, std::forward<Args>(args)...);
    }
};

template <typename NodeT>
regular_sub_pos<NodeT> make_regular_sub_pos(NodeT* node,
                                            shift_t shift,
                                            size_t size)
{
    assert(node);
    assert(shift >= NodeT::bits_leaf);
    assert(size > 0);
    assert(size <= (branches<NodeT::bits> << shift));
    return {node, shift, size};
}

template <typename NodeT>
struct full_pos
{
    static constexpr auto B  = NodeT::bits;
    static constexpr auto BL = NodeT::bits_leaf;

    using node_t = NodeT;
    node_t* node_;
    shift_t shift_;

    count_t count() const { return branches<B>; }
    node_t* node()  const { return node_; }
    size_t  size()  const { return branches<B> << shift_; }
    shift_t shift() const { return shift_; }
    count_t index(size_t idx) const { return (idx >> shift_) & mask<B>; }
    count_t subindex(size_t idx) const { return idx >> shift_; }
    size_t  size(count_t offset) const { return 1 << shift_; }
    size_t  size_sbh(count_t offset, size_t) const { return 1 << shift_; }
    size_t  size_before(count_t offset) const { return offset << shift_; }

    void copy_sizes(count_t offset,
                    count_t n,
                    size_t init,
                    size_t* sizes)
    {
        auto e = sizes + n;
        for (; sizes != e; ++sizes)
            init = *sizes = init + (1 << shift_);
    }

    template <typename Visitor, typename... Args>
    void each(Visitor v, Args&&... args)
    {
        if (shift_ == BL) {
            auto p = node_->inner();
            auto e = p + branches<B>;
            for (; p != e; ++p)
                make_full_leaf_pos(*p).visit(v, args...);
        } else {
            auto p = node_->inner();
            auto e = p + branches<B>;
            auto ss = shift_ - B;
            for (; p != e; ++p)
                make_full_pos(*p, ss).visit(v, args...);
        }
    }

    template <typename Visitor, typename... Args>
    void each_(Visitor v, count_t i, count_t n, Args&&... args)
    {
        auto p = node_->inner() + i;
        auto e = node_->inner() + n;
        if (shift_ == BL) {
            for (; p != e; ++p)
                make_full_leaf_pos(*p).visit(v, args...);
        } else {
            auto ss = shift_ - B;
            for (; p != e; ++p)
                make_full_pos(*p, ss).visit(v, args...);
        }
    }

    template <typename Visitor, typename... Args>
    void each_sub(Visitor v, Args&&... args)
    { each(v, args...); }

    template <typename Visitor, typename... Args>
    void each_left_sub(Visitor v, Args&&... args)
    { each_(v, 0, branches<B> - 1, args...); }

    template <typename Visitor, typename... Args>
    void each_right_sub(Visitor v, Args&&... args)
    { each_(v, 1, branches<B>, args...); }

    template <typename Visitor, typename... Args>
    decltype(auto) towards(Visitor v, size_t idx, Args&&... args)
    { return towards_oh(v, idx, index(idx), args...); }

    template <typename Visitor, typename... Args>
    decltype(auto) towards_oh_ch(Visitor v, size_t idx,
                                 count_t offset_hint, count_t,
                                 Args&&... args)
    { return towards_oh(v, idx, offset_hint, args...); }

    template <typename Visitor, typename... Args>
    decltype(auto) towards_oh(Visitor v, size_t idx,
                              count_t offset_hint,
                              Args&&... args)
    {
        assert(offset_hint == index(idx));
        auto is_leaf = shift_ == BL;
        auto child   = node_->inner() [offset_hint];
        return is_leaf
            ? make_full_leaf_pos(child).visit(v, idx, args...)
            : make_full_pos(child, shift_ - B).visit(v, idx, args...);
    }

    template <typename Visitor, typename... Args>
    decltype(auto) towards_sub_oh(Visitor v, size_t idx,
                                  count_t offset_hint,
                                  Args&&... args)
    {
        assert(offset_hint == index(idx));
        auto is_leaf = shift_ == BL;
        auto child   = node_->inner() [offset_hint];
        auto lsize   = offset_hint << shift_;
        return is_leaf
            ? make_full_leaf_pos(child).visit(v, idx - lsize, args...)
            : make_full_pos(child, shift_ - B).visit(v, idx - lsize, args...);
    }

    template <typename Visitor, typename... Args>
    decltype(auto) first_sub(Visitor v, Args&&... args)
    {
        auto is_leaf = shift_ == BL;
        auto child   = node_->inner() [0];
        return is_leaf
            ? make_full_leaf_pos(child).visit(v, args...)
            : make_full_pos(child, shift_ - B).visit(v, args...);
    }


    template <typename Visitor, typename... Args>
    decltype(auto) first_sub_leaf(Visitor v, Args&&... args)
    {
        assert(shift_ == BL);
        auto child   = node_->inner() [0];
        return make_full_leaf_pos(child).visit(v, args...);
    }

    template <typename Visitor, typename ...Args>
    decltype(auto) visit(Visitor v, Args&& ...args)
    {
        return visit_regular(v, *this, std::forward<Args>(args)...);
    }
};

template <typename NodeT>
full_pos<NodeT> make_full_pos(NodeT* node, shift_t shift)
{
    assert(node);
    assert(shift >= NodeT::bits_leaf);
    return {node, shift};
}

template <typename NodeT>
struct relaxed_pos
{
    static constexpr auto B  = NodeT::bits;
    static constexpr auto BL = NodeT::bits_leaf;

    using node_t = NodeT;
    using relaxed_t = typename NodeT::relaxed_t;
    node_t* node_;
    shift_t shift_;
    relaxed_t* relaxed_;

    count_t count() const { return relaxed_->count; }
    node_t* node()  const { return node_; }
    size_t  size()  const { return relaxed_->sizes[relaxed_->count - 1]; }
    shift_t shift() const { return shift_; }
    count_t subindex(size_t idx) const { return index(idx); }
    relaxed_t* relaxed() const { return relaxed_; }

    size_t size_before(count_t offset) const
    { return offset ? relaxed_->sizes[offset - 1] : 0u; }

    size_t size(count_t offset) const
    { return size_sbh(offset, size_before(offset)); }

    size_t size_sbh(count_t offset, size_t size_before_hint) const
    {
        assert(size_before_hint == size_before(offset));
        return relaxed_->sizes[offset] - size_before_hint;
    }

    count_t index(size_t idx) const
    {
        auto offset = idx >> shift_;
        while (relaxed_->sizes[offset] <= idx) ++offset;
        return offset;
    }

    void copy_sizes(count_t offset,
                    count_t n,
                    size_t init,
                    size_t* sizes)
    {
        auto e = sizes + n;
        auto prev = size_before(offset);
        auto these = relaxed_->sizes + offset;
        for (; sizes != e; ++sizes) {
            init = *sizes = init + (*these - prev);
            prev = *these++;
        }
    }

    template <typename Visitor, typename... Args>
    void each(Visitor v, Args&&... args)
    { each_(v, relaxed_->count, args...); }

    template <typename Visitor, typename... Args>
    void each_sub(Visitor v, Args&&... args)
    { each_(v, relaxed_->count, args...); }

    template <typename Visitor, typename... Args>
    void each_left_sub(Visitor v, Args&&... args)
    { each_(v, relaxed_->count - 1, args...); }

    template <typename Visitor, typename... Args>
    void each_(Visitor v, count_t n, Args&&... args)
    {
        if (shift_ == BL) {
            auto p = node_->inner();
            auto s = size_t{};
            for (auto i = 0u; i < n; ++i) {
                make_leaf_sub_pos(p[i], relaxed_->sizes[i] - s)
                    .visit(v, args...);
                s = relaxed_->sizes[i];
            }
        } else {
            auto p = node_->inner();
            auto s = size_t{};
            auto ss = shift_ - B;
            for (auto i = 0u; i < n; ++i) {
                visit_maybe_relaxed_sub(p[i], ss, relaxed_->sizes[i] - s,
                                        v, args...);
                s = relaxed_->sizes[i];
            }
        }
    }

    template <typename Visitor, typename... Args>
    void each_right_sub(Visitor v, Args&&... args)
    {
        auto s = relaxed_->sizes[0];
        auto p = node_->inner();
        if (shift_ == BL) {
            for (auto i = 1u; i < relaxed_->count; ++i) {
                make_leaf_sub_pos(p[i], relaxed_->sizes[i] - s)
                    .visit(v, args...);
                s = relaxed_->sizes[i];
            }
        } else {
            auto ss = shift_ - B;
            for (auto i = 1u; i < relaxed_->count; ++i) {
                visit_maybe_relaxed_sub(p[i], ss, relaxed_->sizes[i] - s,
                                        v, args...);
                s = relaxed_->sizes[i];
            }
        }
    }

    template <typename Visitor, typename... Args>
    decltype(auto) towards(Visitor v, size_t idx, Args&&... args)
    { return towards_oh(v, idx, subindex(idx), args...); }

    template <typename Visitor, typename... Args>
    decltype(auto) towards_oh(Visitor v, size_t idx,
                              count_t offset_hint,
                              Args&&... args)
    {
        assert(offset_hint == index(idx));
        auto left_size = offset_hint ? relaxed_->sizes[offset_hint - 1] : 0;
        return towards_oh_lsh(v, idx, offset_hint, left_size, args...);
    }

    template <typename Visitor, typename... Args>
    decltype(auto) towards_oh_lsh(Visitor v, size_t idx,
                                  count_t offset_hint,
                                  size_t left_size_hint,
                                  Args&&... args)
    { return towards_sub_oh_lsh(v, idx, offset_hint, left_size_hint, args...); }

    template <typename Visitor, typename... Args>
    decltype(auto) towards_sub_oh(Visitor v, size_t idx,
                                  count_t offset_hint,
                                  Args&&... args)
    {
        assert(offset_hint == index(idx));
        auto left_size = offset_hint ? relaxed_->sizes[offset_hint - 1] : 0;
        return towards_sub_oh_lsh(v, idx, offset_hint, left_size, args...);
    }

    template <typename Visitor, typename... Args>
    decltype(auto) towards_sub_oh_lsh(Visitor v, size_t idx,
                                      count_t offset_hint,
                                      size_t left_size_hint,
                                      Args&&... args)
    {
        assert(offset_hint == index(idx));
        assert(left_size_hint ==
               (offset_hint ? relaxed_->sizes[offset_hint - 1] : 0));
        auto child     = node_->inner() [offset_hint];
        auto is_leaf   = shift_ == BL;
        auto next_size = relaxed_->sizes[offset_hint] - left_size_hint;
        auto next_idx  = idx - left_size_hint;
        return is_leaf
            ? make_leaf_sub_pos(child, next_size).visit(
                v, next_idx, args...)
            : visit_maybe_relaxed_sub(child, shift_ - B, next_size,
                                      v, next_idx, args...);
    }

    template <typename Visitor, typename... Args>
    decltype(auto) last_oh_csh(Visitor v,
                               count_t offset_hint,
                               size_t child_size_hint,
                               Args&&... args)
    {
        assert(offset_hint == count() - 1);
        assert(child_size_hint == size(offset_hint));
        auto child     = node_->inner() [offset_hint];
        auto is_leaf   = shift_ == BL;
        return is_leaf
            ? make_leaf_sub_pos(child, child_size_hint).visit(v, args...)
            : visit_maybe_relaxed_sub(child, shift_ - B, child_size_hint,
                                      v, args...);
    }

    template <typename Visitor, typename... Args>
    decltype(auto) last_sub(Visitor v, Args&&... args)
    {
        auto offset     = relaxed_->count - 1;
        auto child      = node_->inner() [offset];
        auto child_size = size(offset);
        auto is_leaf    = shift_ == BL;
        return is_leaf
            ? make_leaf_sub_pos(child, child_size).visit(v, args...)
            : visit_maybe_relaxed_sub(child, shift_ - B, child_size, v, args...);
    }

    template <typename Visitor, typename... Args>
    decltype(auto) first_sub(Visitor v, Args&&... args)
    {
        auto child      = node_->inner() [0];
        auto child_size = relaxed_->sizes[0];
        auto is_leaf    = shift_ == BL;
        return is_leaf
            ? make_leaf_sub_pos(child, child_size).visit(v, args...)
            : visit_maybe_relaxed_sub(child, shift_ - B, child_size, v, args...);
    }

    template <typename Visitor, typename... Args>
    decltype(auto) first_sub_leaf(Visitor v, Args&&... args)
    {
        assert(shift_ == BL);
        auto child      = node_->inner() [0];
        auto child_size = relaxed_->sizes[0];
        return make_leaf_sub_pos(child, child_size).visit(v, args...);
    }

    template <typename Visitor, typename ...Args>
    decltype(auto) visit(Visitor v, Args&& ...args)
    {
        return visit_relaxed(v, *this, std::forward<Args>(args)...);
    }
};

template <typename NodeT>
relaxed_pos<NodeT> make_relaxed_pos(NodeT* node,
                                    shift_t shift,
                                    typename NodeT::relaxed_t* relaxed)
{
    assert(node);
    assert(relaxed);
    assert(shift >= NodeT::bits_leaf);
    return {node, shift, relaxed};
}

template <typename NodeT, typename Visitor, typename... Args>
decltype(auto) visit_maybe_relaxed_sub(NodeT* node, shift_t shift, size_t size,
                                       Visitor v, Args&& ...args)
{
    assert(node);
    auto relaxed = node->relaxed();
    if (relaxed) {
        assert(size == relaxed->sizes[relaxed->count - 1]);
        return make_relaxed_pos(node, shift, relaxed)
            .visit(v, std::forward<Args>(args)...);
    } else {
        return make_regular_sub_pos(node, shift, size)
            .visit(v, std::forward<Args>(args)...);
    }
}

} // namespace rbts
} // namespace detail
} // namespace immer
