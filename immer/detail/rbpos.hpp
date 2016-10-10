
#pragma once

#include <immer/config.hpp>
#include <immer/detail/rbbits.hpp>

#include <utility>
#include <cassert>

namespace immer {
namespace detail {

template <typename NodeT>
struct empty_regular_rbpos
{
    static constexpr auto bits = NodeT::bits;
    using node_t = NodeT;
    node_t* node_;

    auto count() const { return 0; }
    auto node()  const { return node_; }
    auto shift() const { return 0; }
    auto size()  const { return 0; }

    template <typename Visitor>
    void each(Visitor&&) {}

    template <typename Visitor>
    auto visit(Visitor&& v)
    {
        return v.visit_inner(*this, v);
    }
};

template <typename NodeT>
empty_regular_rbpos<NodeT> make_empty_regular_rbpos(NodeT* node)
{
    assert(node);
    return {node};
}

template <typename NodeT>
struct empty_leaf_rbpos
{
    static constexpr auto bits = NodeT::bits;
    using node_t = NodeT;
    node_t* node_;

    auto count() const { return 0; }
    auto node()  const { return node_; }
    auto shift() const { return 0; }
    auto size()  const { return 0; }

    template <typename Visitor, typename ...Args>
    auto visit(Visitor&& v, Args&& ...args)
    {
        return v.visit_leaf(*this, v, std::forward<Args>(args)...);
    }
};

template <typename NodeT>
empty_leaf_rbpos<NodeT> make_empty_leaf_rbpos(NodeT* node)
{
    assert(node);
    return {node};
}

template <typename NodeT>
struct leaf_rbpos
{
    static constexpr auto bits = NodeT::bits;
    using node_t = NodeT;
    node_t* node_;
    unsigned count_;

    auto count() const { return count_; }
    auto node()  const { return node_; }
    auto size()  const { return count_; }
    auto shift() const { return 0; }
    auto index(std::size_t idx) const { return idx & mask<bits>; }

    template <typename Visitor, typename ...Args>
    auto visit(Visitor&& v, Args&& ...args)
    {
        return v.visit_leaf(*this, v, std::forward<Args>(args)...);
    }
};

template <typename NodeT>
leaf_rbpos<NodeT> make_leaf_rbpos(NodeT* node, unsigned count)
{
    assert(node);
    assert(count > 0);
    return {node, count};
}

template <typename NodeT>
struct leaf_descent_rbpos
{
    static constexpr auto bits = NodeT::bits;
    using node_t = NodeT;
    node_t* node_;

    auto node()  const { return node_; }
    auto shift() const { return 0; }
    auto index(std::size_t idx) const { return idx & mask<bits>; }

    template <typename... Args>
    auto descend(Args&&...) {}

    template <typename Visitor, typename ...Args>
    auto visit(Visitor&& v, Args&& ...args)
    {
        return v.visit_leaf(*this, v, std::forward<Args>(args)...);
    }
};

template <typename NodeT>
leaf_descent_rbpos<NodeT> make_leaf_descent_rbpos(NodeT* node)
{
    assert(node);
    return {node};
}

template <typename NodeT>
struct full_leaf_rbpos
{
    static constexpr auto bits = NodeT::bits;
    using node_t = NodeT;
    node_t* node_;

    auto count() const { return branches<bits>; }
    auto node()  const { return node_; }
    auto size()  const { return branches<bits>; }
    auto shift() const { return 0; }
    auto index(std::size_t idx) const { return idx & mask<bits>; }

    template <typename Visitor, typename ...Args>
    auto visit(Visitor&& v, Args&& ...args)
    {
        return v.visit_leaf(*this, v, std::forward<Args>(args)...);
    }
};

template <typename NodeT>
full_leaf_rbpos<NodeT> make_full_leaf_rbpos(NodeT* node)
{
    assert(node);
    return {node};
}

template <typename NodeT>
struct regular_rbpos
{
    static constexpr auto bits = NodeT::bits;
    using node_t = NodeT;
    node_t* node_;
    unsigned shift_;
    std::size_t size_;

    auto count() const { return index(size_ - 1) + 1; }
    auto node()  const { return node_; }
    auto size()  const { return size_; }
    auto shift() const { return shift_; }
    auto index(std::size_t idx) const { return (idx >> shift_) & mask<bits>; }

    template <typename Visitor>
    void each(Visitor&& v)
    {
        if (shift_ == bits) {
            auto p = node_->inner();
            auto e = p + index(size_ - 1);
            for (; p != e; ++p)
                make_full_leaf_rbpos(*p).visit(v);
            make_leaf_rbpos(*p, ((size_ - 1) & mask<bits>) + 1).visit(v);
        } else {
            auto p = node_->inner();
            auto e = p + index(size_ - 1);
            auto ss = shift_ - bits;
            for (; p != e; ++p)
                make_full_rbpos(*p, ss).visit(v);
            make_regular_rbpos(*p, ss, size_).visit(v);
        }
    }

    template <typename Visitor>
    auto towards(Visitor&& v, std::size_t idx)
    {
        return towards(v, idx, index(idx), count());
    }

    template <typename Visitor>
    auto towards(Visitor&& v, std::size_t idx, unsigned offset_hint)
    {
        return towards(v, idx, offset_hint, count());
    }

    template <typename Visitor>
    auto towards(Visitor&& v,
                 std::size_t idx,
                 unsigned offset_hint,
                 unsigned count_hint)
    {
        assert(offset_hint == index(idx));
        assert(count_hint  == count());
        auto is_last = offset_hint + 1 == count_hint;
        auto is_leaf = shift_ == bits;
        auto child   = node_->inner() [offset_hint];
        return is_last
            ? (is_leaf
               ? make_leaf_rbpos(child, ((size_ - 1) & mask<bits>) + 1).visit(v, idx)
               : make_regular_rbpos(child, shift_ - bits, size_).visit(v, idx))
            : (is_leaf
               ? make_full_leaf_rbpos(child).visit(v, idx)
               : make_full_rbpos(child, shift_ - bits).visit(v, idx));
    }

    template <typename Visitor>
    auto last(Visitor&& v, unsigned offset_hint)
    {
        assert(offset_hint == count() - 1);
        auto child   = node_->inner() [offset_hint];
        auto is_leaf = shift_ == bits;
        return is_leaf
            ? make_leaf_rbpos(child, ((size_ - 1) & mask<bits>) + 1).visit(v)
            : make_regular_rbpos(child, shift_ - bits, size_).visit(v);
    }

    template <typename Visitor, typename ...Args>
    auto visit(Visitor&& v, Args&& ...args)
    {
        return v.visit_inner(*this, v, std::forward<Args>(args)...);
    }
};

template <typename NodeT>
regular_rbpos<NodeT> make_regular_rbpos(NodeT* node,
                                        unsigned shift,
                                        std::size_t size)
{
    assert(node);
    assert(shift >= NodeT::bits);
    assert(size > 0);
    return {node, shift, size};
}

template <typename NodeT, unsigned Shift, unsigned B=NodeT::bits>
struct regular_descent_rbpos
{
    static_assert(Shift > 0, "not leaf...");

    using node_t = NodeT;
    node_t* node_;

    auto node()  const { return node_; }
    auto shift() const { return Shift; }
    auto index(std::size_t idx) const { return (idx >> Shift) & mask<B>; }

    template <typename Visitor>
    auto descend(Visitor&& v, std::size_t idx)
    {
        auto offset = index(idx);
        auto child  = node_->inner()[offset];
        return regular_descent_rbpos<NodeT, Shift - B>{child}.visit(v, idx);
    }

    template <typename Visitor, typename ...Args>
    auto visit(Visitor&& v, Args&& ...args)
    {
        return v.visit_inner(*this, v, std::forward<Args>(args)...);
    }
};

template <typename NodeT, unsigned B>
struct regular_descent_rbpos<NodeT, B, B>
{
    using node_t = NodeT;
    node_t* node_;

    auto node()  const { return node_; }
    auto shift() const { return B; }
    auto index(std::size_t idx) const { return (idx >> B) & mask<B>; }

    template <typename Visitor>
    auto descend(Visitor&& v, std::size_t idx)
    {
        auto offset = this->index(idx);
        auto child  = this->node_->inner()[offset];
        return make_leaf_descent_rbpos(child).visit(v, idx);
    }

    template <typename Visitor, typename ...Args>
    auto visit(Visitor&& v, Args&& ...args)
    {
        return v.visit_inner(*this, v, std::forward<Args>(args)...);
    }
};

template <typename NodeT, typename Visitor>
auto visit_regular_descent(NodeT* node, unsigned shift, Visitor&& v,
                           std::size_t idx)
{
    constexpr auto B = NodeT::bits;
    assert(node);
    assert(shift >= B);
    switch (shift) {
    case B * 1: return regular_descent_rbpos<NodeT, B * 1>{node}.visit(v, idx);
    case B * 2: return regular_descent_rbpos<NodeT, B * 2>{node}.visit(v, idx);
    case B * 3: return regular_descent_rbpos<NodeT, B * 3>{node}.visit(v, idx);
    case B * 4: return regular_descent_rbpos<NodeT, B * 4>{node}.visit(v, idx);
    case B * 5: return regular_descent_rbpos<NodeT, B * 5>{node}.visit(v, idx);
    case B * 6: return regular_descent_rbpos<NodeT, B * 6>{node}.visit(v, idx);
    default:    return leaf_descent_rbpos<NodeT>{node}.visit(v, idx);
    }
}

template <typename NodeT>
struct full_rbpos
{
    static constexpr auto bits = NodeT::bits;
    using node_t = NodeT;
    node_t* node_;
    unsigned shift_;

    auto count() const { return branches<bits>; }
    auto node()  const { return node_; }
    auto size()  const { return 1 << shift_; }
    auto shift() const { return shift_; }
    auto index(std::size_t idx) const { return (idx >> shift_) & mask<bits>; }

    template <typename Visitor>
    void each(Visitor&& v)
    {
        if (shift_ == bits) {
            auto p = node_->inner();
            auto e = p + branches<bits>;
            for (; p != e; ++p)
                make_full_leaf_rbpos(*p).visit(v);
        } else {
            auto p = node_->inner();
            auto e = p + branches<bits>;
            auto ss = shift_ - bits;
            for (; p != e; ++p)
                make_full_rbpos(*p, ss).visit(v);
        }
    }

    template <typename Visitor>
    auto towards(Visitor&& v, std::size_t idx)
    {
        return towards(v, idx, index(idx));
    }

    template <typename Visitor, typename... OtherHints>
    auto towards(Visitor&& v, std::size_t idx,
                 unsigned offset_hint,
                 OtherHints&&...)
    {
        assert(offset_hint == index(idx));
        auto is_leaf = shift_ == bits;
        auto child   = node_->inner() [offset_hint];
        return is_leaf
            ? make_full_leaf_rbpos(child).visit(v, idx)
            : make_full_rbpos(child, shift_ - bits).visit(v, idx);
    }

    template <typename Visitor, typename ...Args>
    auto visit(Visitor&& v, Args&& ...args)
    {
        return v.visit_inner(*this, v, std::forward<Args>(args)...);
    }
};

template <typename NodeT>
full_rbpos<NodeT> make_full_rbpos(NodeT* node,
                                  unsigned shift)
{
    assert(node);
    assert(shift >= NodeT::bits);
    return {node, shift};
}

template <typename NodeT>
struct relaxed_rbpos
{
    static constexpr auto bits = NodeT::bits;
    using node_t = NodeT;
    using relaxed_t = typename NodeT::relaxed_t;
    node_t* node_;
    unsigned shift_;
    relaxed_t* relaxed_;

    auto count() const { return relaxed_->count; }
    auto node()  const { return node_; }
    auto size()  const { return relaxed_->sizes[relaxed_->count - 1]; }
    auto shift() const { return shift_; }
    auto relaxed() const { return relaxed_; }

    auto child_size(unsigned offset) const
    {
        return offset
            ? relaxed_->sizes[offset] - relaxed_->sizes[offset - 1]
            : relaxed_->sizes[offset];
    }

    auto index(std::size_t idx) const
    {
        auto offset = (idx >> shift_) & mask<bits>;
        while (relaxed_->sizes[offset] <= idx) ++offset;
        return offset;
    }

    template <typename Visitor>
    void each(Visitor&& v)
    {
        if (shift_ == bits) {
            auto p = node_->inner();
            auto s = std::size_t{};
            for (auto i = 0u; i < relaxed_->count; ++i) {
                make_leaf_rbpos(p[i], relaxed_->sizes[i] - s).visit(v);
                s = relaxed_->sizes[i];
            }
        } else {
            auto p = node_->inner();
            auto s = std::size_t{};
            auto ss = shift_ - bits;
            for (auto i = 0u; i < relaxed_->count; ++i) {
                visit_maybe_relaxed(p[i], ss, relaxed_->sizes[i] - s, v);
                s = relaxed_->sizes[i];
            }
        }
    }

    template <typename Visitor>
    auto towards(Visitor&& v,
                 std::size_t idx,
                 unsigned offset_hint)
    {
        assert(offset_hint == index(idx));
        auto left_size = offset_hint ? relaxed_->sizes[offset_hint - 1] : 0;
        return towards(v, idx, offset_hint, left_size);
    }

    template <typename Visitor>
    auto towards(Visitor&& v,
                 std::size_t idx,
                 unsigned offset_hint,
                 std::size_t left_size_hint)
    {
        assert(offset_hint == index(idx));
        assert(left_size_hint ==
               (offset_hint ? relaxed_->sizes[offset_hint - 1] : 0));
        auto child     = node_->inner() [offset_hint];
        auto is_leaf   = shift_ == bits;
        auto next_size = relaxed_->sizes[offset_hint] - left_size_hint;
        auto next_idx  = idx - left_size_hint;
        return is_leaf
            ? make_leaf_rbpos(child, next_size).visit(v, next_idx)
            : visit_maybe_relaxed(child, shift_ - bits, next_size, v, next_idx);
    }

    template <typename Visitor>
    auto last(Visitor&& v, unsigned offset_hint, unsigned child_size_hint)
    {
        assert(offset_hint == count() - 1);
        assert(child_size_hint == child_size(offset_hint));
        auto child     = node_->inner() [offset_hint];
        auto is_leaf   = shift_ == bits;
        return is_leaf
            ? make_leaf_rbpos(child, child_size_hint).visit(v)
            : visit_maybe_relaxed(child, shift_ - bits, child_size_hint, v);
    }

    template <typename Visitor, typename ...Args>
    auto visit(Visitor&& v, Args&& ...args)
    {
        return v.visit_relaxed(*this, v, std::forward<Args>(args)...);
    }
};

template <typename NodeT>
relaxed_rbpos<NodeT> make_relaxed_rbpos(NodeT* node,
                                        unsigned shift,
                                        typename NodeT::relaxed_t* relaxed)
{
    assert(node);
    assert(relaxed);
    assert(shift >= NodeT::bits);
    return {node, shift, relaxed};
}

template <typename NodeT, typename Visitor, typename... Args>
auto visit_maybe_relaxed(NodeT* node, unsigned shift, std::size_t size,
                         Visitor&& v, Args&& ...args)
{
    assert(node);
    auto relaxed = node->relaxed();
    if (relaxed) {
        assert(size == relaxed->sizes[relaxed->count - 1]);
        return make_relaxed_rbpos(node, shift, relaxed)
            .visit(v, std::forward<Args>(args)...);
    } else {
        return make_regular_rbpos(node, shift, size)
            .visit(v, std::forward<Args>(args)...);
    }
}

template <typename NodeT, unsigned Shift, unsigned B=NodeT::bits>
struct relaxed_descent_rbpos
{
    static_assert(Shift > 0, "not leaf...");

    static constexpr auto bits = NodeT::bits;
    using node_t = NodeT;
    using relaxed_t = typename NodeT::relaxed_t;
    node_t* node_;
    relaxed_t* relaxed_;

    auto count() const { return relaxed_->count; }
    auto node()  const { return node_; }
    auto shift() const { return Shift; }
    auto size()  const { return relaxed_->sizes[relaxed_->count - 1]; }

    auto index(std::size_t idx) const
    {
        auto offset = (idx >> Shift) & mask<bits>;
        while (relaxed_->sizes[offset] <= idx) ++offset;
        return offset;
    }

    template <typename Visitor>
    auto descend(Visitor&& v, std::size_t idx)
    {
        auto offset    = index(idx);
        auto child     = node_->inner() [offset];
        auto left_size = offset ? relaxed_->sizes[offset - 1] : 0;
        auto next_idx  = idx - left_size;
        auto r  = child->relaxed();
        return r
            ? relaxed_descent_rbpos<NodeT, Shift - B>{child, r}.visit(v, next_idx)
            : regular_descent_rbpos<NodeT, Shift - B>{child}.visit(v, next_idx);
    }

    template <typename Visitor, typename ...Args>
    auto visit(Visitor&& v, Args&& ...args)
    {
        return v.visit_relaxed(*this, v, std::forward<Args>(args)...);
    }
};

template <typename NodeT, unsigned B>
struct relaxed_descent_rbpos<NodeT, B, B>
{
    static constexpr auto bits = NodeT::bits;
    using node_t = NodeT;
    using relaxed_t = typename NodeT::relaxed_t;
    node_t* node_;
    relaxed_t* relaxed_;

    auto count() const { return relaxed_->count; }
    auto node()  const { return node_; }
    auto shift() const { return B; }
    auto size()  const { return relaxed_->sizes[relaxed_->count - 1]; }

    auto index(std::size_t idx) const
    {
        auto offset = (idx >> B) & mask<bits>;
        while (relaxed_->sizes[offset] <= idx) ++offset;
        return offset;
    }

    template <typename Visitor>
    auto descend(Visitor&& v, std::size_t idx)
    {
        auto offset    = this->index(idx);
        auto child     = this->node_->inner() [offset];
        auto left_size = offset ? this->relaxed_->sizes[offset - 1] : 0;
        auto next_idx  = idx - left_size;
        return leaf_descent_rbpos<NodeT>{child}.visit(v, next_idx);
    }

    template <typename Visitor, typename ...Args>
    auto visit(Visitor&& v, Args&& ...args)
    {
        return v.visit_relaxed(*this, v, std::forward<Args>(args)...);
    }
};

template <typename NodeT, typename Visitor, typename... Args>
auto visit_maybe_relaxed_descent(NodeT* node, unsigned shift,
                                 Visitor&& v, std::size_t idx)
{
    constexpr auto B = NodeT::bits;
    assert(node);
    assert(shift >= B);
    auto r = node->relaxed();
    if (r) {
        switch (shift) {
        case B * 1: return relaxed_descent_rbpos<NodeT, B * 1>{node, r}.visit(v, idx);
        case B * 2: return relaxed_descent_rbpos<NodeT, B * 2>{node, r}.visit(v, idx);
        case B * 3: return relaxed_descent_rbpos<NodeT, B * 3>{node, r}.visit(v, idx);
        case B * 4: return relaxed_descent_rbpos<NodeT, B * 4>{node, r}.visit(v, idx);
        case B * 5: return relaxed_descent_rbpos<NodeT, B * 5>{node, r}.visit(v, idx);
        case B * 6: return relaxed_descent_rbpos<NodeT, B * 6>{node, r}.visit(v, idx);
        default:    return leaf_descent_rbpos<NodeT>{node}.visit(v, idx);
        }
    } else {
        return visit_regular_descent(node, shift, v, idx);
    }
}

template <typename... Fns>
struct fn_visitor;

template <typename Fn>
struct fn_visitor<Fn> : std::tuple<Fn>
{
    using base_t = std::tuple<Fn>;
    using base_t::base_t;

    template <typename... Args>
    auto visit_relaxed(Args&& ...args)
    { return std::get<0>(*this)(std::forward<Args>(args)...); }

    template <typename... Args>
    auto visit_inner(Args&& ...args)
    { return std::get<0>(*this)(std::forward<Args>(args)...); }

    template <typename... Args>
    auto visit_leaf(Args&& ...args)
    { return std::get<0>(*this)(std::forward<Args>(args)...); }
};

template <typename FnI, typename FnL>
struct fn_visitor<FnI, FnL> : std::tuple<FnI, FnL>
{
    using base_t = std::tuple<FnI, FnL>;
    using base_t::base_t;

    template <typename... Args>
    auto visit_relaxed(Args&& ...args)
    { return std::get<0>(*this)(std::forward<Args>(args)...); }

    template <typename... Args>
    auto visit_inner(Args&& ...args)
    { return std::get<0>(*this)(std::forward<Args>(args)...); }

    template <typename... Args>
    auto visit_leaf(Args&& ...args)
    { return std::get<1>(*this)(std::forward<Args>(args)...); }
};

template <typename FnR, typename FnI, typename FnL>
struct fn_visitor<FnR, FnI, FnL> : std::tuple<FnR, FnI, FnL>
{
    using base_t = std::tuple<FnR, FnI, FnL>;
    using base_t::base_t;

    template <typename... Args>
    auto visit_relaxed(Args&& ...args)
    { return std::get<0>(*this)(std::forward<Args>(args)...); }

    template <typename... Args>
    auto visit_inner(Args&& ...args)
    { return std::get<1>(*this)(std::forward<Args>(args)...); }

    template <typename... Args>
    auto visit_leaf(Args&& ...args)
    { return std::get<2>(*this)(std::forward<Args>(args)...); }
};

template <typename... Fns>
auto make_visitor(Fns&& ...fns)
    -> fn_visitor<std::decay_t<Fns>...>
{
    return { std::forward<Fns>(fns)... };
}

} // namespace detail
} // namespace immer
