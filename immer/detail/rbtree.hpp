
#pragma once

#include <immer/detail/rbnode.hpp>
#include <immer/detail/rbpos.hpp>
#include <immer/detail/rbalgorithm.hpp>

#include <cassert>
#include <memory>
#include <numeric>

namespace immer {
namespace detail {

template <typename T,
          int B,
          typename MemoryPolicy>
struct rbtree
{
    using heap_policy = typename MemoryPolicy::heap;
    using refcount    = typename MemoryPolicy::refcount;

    using node_t = rbnode<T, B, MemoryPolicy>;
    using heap   = typename node_t::heap;

    std::size_t size;
    unsigned    shift;
    node_t*     root;
    node_t*     tail;

    static const rbtree empty;

    rbtree(std::size_t sz, unsigned sh, node_t* r, node_t* t)
        : size{sz}, shift{sh}, root{r}, tail{t}
    {}

    rbtree(const rbtree& other)
        : rbtree{other.size, other.shift, other.root, other.tail}
    {
        inc();
    }

    rbtree(rbtree&& other)
        : rbtree{empty}
    {
        swap(*this, other);
    }

    rbtree& operator=(const rbtree& other)
    {
        auto next = other;
        swap(*this, next);
        return *this;
    }

    rbtree& operator=(rbtree&& other)
    {
        swap(*this, other);
        return *this;
    }

    friend void swap(rbtree& x, rbtree& y)
    {
        using std::swap;
        swap(x.size,  y.size);
        swap(x.shift, y.shift);
        swap(x.root,  y.root);
        swap(x.tail,  y.tail);
    }

    ~rbtree()
    {
        dec();
    }

    void inc() const
    {
        root->inc();
        tail->inc();
    }

    void dec() const
    {
        traverse(dec_visitor());
    }

    auto tail_size() const
    {
        return size - tail_offset();
    }

    auto tail_offset() const
    {
        return size ? (size-1) & ~mask<B> : 0;
    }

    template <typename Visitor>
    void traverse(Visitor&& v) const
    {
        auto tail_off  = tail_offset();
        auto tail_size = size - tail_off;

        if (tail_off) make_regular_rbpos(root, shift, tail_off).visit(v);
        else make_empty_regular_rbpos(root).visit(v);

        if (tail_size) make_leaf_rbpos(tail, tail_size).visit(v);
        else make_empty_leaf_rbpos(tail).visit(v);
    }

    template <typename Visitor>
    auto descend(Visitor&& v, std::size_t idx) const
    {
        auto tail_off  = tail_offset();
        return idx >= tail_off
            ? make_leaf_descent_rbpos(tail).visit(v, idx)
            : visit_regular_descent(root, shift, v, idx);
    }

    template <typename Step, typename State>
    State reduce(Step step, State acc) const
    {
        traverse(reduce_visitor(step, acc));
        return acc;
    }

    rbtree push_back(T value) const
    {
        auto tail_off = tail_offset();
        auto ts = size - tail_off;
        if (ts < branches<B>) {
            auto new_tail = node_t::copy_leaf_emplace(tail, ts,
                                                      std::move(value));
            return { size + 1, shift, root->inc(), new_tail };
        } else {
            auto new_tail = node_t::make_leaf(std::move(value));
            if ((size >> B) > (1u << shift)) {
                auto new_root = node_t::make_inner(
                    root->inc(),
                    node_t::make_path(shift, tail->inc()));
                return { size + 1, shift + B, new_root, new_tail };
            } else if (tail_off) {
                auto new_root = make_regular_rbpos(root, shift, tail_off)
                    .visit(push_tail_visitor(tail->inc(), ts));
                return { size + 1, shift, new_root, new_tail };
            } else {
                auto new_root = node_t::make_path(shift, tail->inc());
                return { size + 1, shift, new_root, new_tail };
            }
        }
    }

    const T* array_for(std::size_t index) const
    {
        return descend(array_for_visitor<T>(), index);
    }

    const T& get(std::size_t index) const
    {
        return *descend(get_visitor<T>(), index);
    }

    template <typename FnT>
    rbtree update(std::size_t idx, FnT&& fn) const
    {
        auto tail_off  = tail_offset();
        auto visitor   = update_visitor<node_t>(std::forward<FnT>(fn));
        if (idx >= tail_off) {
            auto tail_size = size - tail_off;
            auto new_tail  = make_leaf_rbpos(tail, tail_size)
                .visit(visitor, idx);
            return { size, shift, root->inc(), new_tail };
        } else {
            auto new_root  = make_regular_rbpos(root, shift, tail_off)
                .visit(visitor, idx);
            return { size, shift, new_root, tail->inc() };
        }
    }

    template <typename FnT>
    node_t* do_update_full(unsigned level,
                           node_t* node,
                           std::size_t idx,
                           FnT&& fn) const
    {
        if (level == 0) {
            auto new_node  = node_t::copy_leaf(node, branches<B>);
            auto& item     = new_node->leaf() [idx & mask<B>];
            auto new_value = std::forward<FnT>(fn) (std::move(item));
            item = std::move(new_value);
            return new_node;
        } else {
            auto offset   = (idx >> level) & mask<B>;
            auto new_node = node_t::copy_inner(node, branches<B>);
            node->inner()[offset]->dec_unsafe();
            new_node->inner()[offset] =
                do_update_full(level - B, node->inner()[offset], idx,
                               std::forward<FnT>(fn));
            return new_node;
        }
    }

    template <typename FnT>
    node_t* do_update_last(unsigned level,
                           node_t* node,
                           std::size_t idx,
                           FnT&& fn) const
    {
        if (level == 0) {
            auto new_node  = node_t::copy_leaf(node, branches<B>);
            auto& item     = new_node->leaf() [idx & mask<B>];
            auto new_value = std::forward<FnT>(fn) (std::move(item));
            item = std::move(new_value);
            return new_node;
        } else {
            auto offset     = (idx >> level) & mask<B>;
            auto end_offset = ((tail_offset()-1) >> level) & mask<B>;
            auto new_node   = node_t::copy_inner(node, end_offset + 1);
            node->inner()[offset]->dec_unsafe();
            new_node->inner()[offset] =
                offset == end_offset
                ? do_update_last(level - B, node->inner()[offset], idx,
                                 std::forward<FnT>(fn))
                : do_update_full(level - B, node->inner()[offset], idx,
                                 std::forward<FnT>(fn));
            return new_node;
        }
    }

    rbtree assoc(std::size_t idx, T value) const
    {
        return update(idx, [&] (auto&&) {
                return std::move(value);
            });
    }

    rbtree take(std::size_t new_size) const
    {
        auto tail_off = tail_offset();
        if (new_size == 0) {
            return empty;
        } else if (new_size >= size) {
            return *this;
        } else if (new_size > tail_off) {
            auto new_tail = node_t::copy_leaf(tail, new_size - tail_off);
            return { new_size, shift, root->inc(), new_tail };
        } else {
            using std::get;
            auto l = new_size - 1;
            auto v = slice_right_visitor<node_t>();
            auto r = make_regular_rbpos(root, shift, tail_off).visit(v, l);
            auto new_shift = get<0>(r);
            auto new_root  = get<1>(r);
            auto new_tail  = get<3>(r);
            if (new_root) {
                assert(new_root->compute_shift() == get<0>(r));
                assert(new_root->check(new_shift, new_size - get<2>(r)));
                return { new_size, new_shift, new_root, new_tail };
            } else {
                return { new_size, B, empty.root->inc(), new_tail };
            }
        }
    }
};

template <typename T, int B, typename MP>
const rbtree<T, B, MP> rbtree<T, B, MP>::empty = {
    0,
    B,
    node_t::make_inner(),
    node_t::make_leaf()
};

} /* namespace detail */
} /* namespace immer */
