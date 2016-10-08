
#pragma once

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
    void descend(Visitor&&, std::size_t) {}

    template <typename Visitor>
    auto visit(Visitor&& v)
    {
        return v([&] (auto&& vr, auto&& vi, auto&& vl) {
            return vi(*this, v);
        });
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

    template <typename ...Visitor>
    void each(Visitor&&...) {}
    template <typename Visitor>
    auto descend(Visitor&&, std::size_t) {}

    template <typename Visitor, typename ...Args>
    auto visit(Visitor&& v, Args&& ...args)
    {
        return v([&] (auto&& vr, auto&& vi, auto&& vl) {
            return vl(*this, v, std::forward<Args>(args)...);
        });
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

    template <typename ...Visitor>
    void each(Visitor&&...) {}
    template <typename Visitor>
    auto descend(Visitor&&, std::size_t) {}

    template <typename Visitor, typename ...Args>
    auto visit(Visitor&& v, Args&& ...args)
    {
        return v([&] (auto&& vr, auto&& vi, auto&& vl) {
            return vl(*this, v, std::forward<Args>(args)...);
        });
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

    template <typename ...Visitor>
    void each(Visitor&&...) {}
    template <typename Visitor>
    auto descend(Visitor&&, std::size_t) {}

    template <typename Visitor, typename ...Args>
    auto visit(Visitor&& v, Args&& ...args)
    {
        return v([&] (auto&& vr, auto&& vi, auto&& vl) {
            return vl(*this, v, std::forward<Args>(args)...);
        });
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
    auto descend(Visitor&& v, std::size_t idx)
    {
        auto offset  = index(idx);
        auto is_last = offset + 1 == count();
        auto is_leaf = shift_ == bits;
        auto child   = node_->inner() [offset];
        return is_last
            ? (is_leaf
               ? make_leaf_rbpos(child, ((size_ - 1) & mask<bits>) + 1).visit(v, idx)
               : make_regular_rbpos(child, shift_ - bits, size_).visit(v, idx))
            : (is_leaf
               ? make_full_leaf_rbpos(child).visit(v, idx)
               : make_full_rbpos(child, shift_ - bits).visit(v, idx));
    }

    template <typename Visitor, typename ...Args>
    auto visit(Visitor&& v, Args&& ...args)
    {
        return v([&] (auto&& vr, auto&& vi, auto&& vl) {
            return vi(*this, v, std::forward<Args>(args)...);
        });
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
    auto descend(Visitor&& v, std::size_t idx)
    {
        auto offset  = index(idx);
        auto is_leaf = shift_ == bits;
        auto child   = node_->inner() [offset];
        return is_leaf
            ? make_full_leaf_rbpos(child).visit(v, idx)
            : make_full_rbpos(child, shift_ - bits).visit(v, idx);
    }

    template <typename Visitor, typename ...Args>
    auto visit(Visitor&& v, Args&& ...args)
    {
        return v([&] (auto&& vr, auto&& vi, auto&& vl) {
            return vi(*this, v, std::forward<Args>(args)...);
        });
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
    auto descend(Visitor&& v, std::size_t idx)
    {
        auto offset    = index(idx);
        auto child     = node_->inner() [offset];
        auto is_leaf   = shift_ == bits;
        auto left_size = offset ? relaxed_->sizes[offset - 1] : 0;
        auto next_size = relaxed_->sizes[offset] - left_size;
        auto next_idx  = idx - left_size;
        return is_leaf
            ? make_leaf_rbpos(child, next_size).visit(v, next_idx)
            : visit_maybe_relaxed(child, shift_ - bits, next_size, v, next_idx);
    }

    template <typename Visitor, typename ...Args>
    auto visit(Visitor&& v, Args&& ...args)
    {
        return v([&] (auto&& vr, auto&& vi, auto&& vl) {
            return vr(*this, v, std::forward<Args>(args)...);
        });
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

template <typename GenericVisitor>
auto make_visitor(GenericVisitor v)
{
    return
        [=] (auto&& op) {
            return std::forward<decltype(op)>(op) (
                v, v, v);
        };
}

template <typename InnerVisitor, typename LeafVisitor>
auto make_visitor(InnerVisitor vi, LeafVisitor vl)
{
    return
        [=] (auto&& op) {
            return std::forward<decltype(op)>(op) (
                vi, vi, vl);
        };
}

template <typename RelaxedVisitor, typename InnerVisitor, typename LeafVisitor>
auto make_visitor(RelaxedVisitor vr, InnerVisitor vi, LeafVisitor vl)
{
    return
        [=] (auto&& op) {
            return std::forward<decltype(op)>(op) (
                vr, vi, vl);
        };
}

} // namespace detail
} // namespace immer
