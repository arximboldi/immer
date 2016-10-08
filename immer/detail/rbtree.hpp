
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
        do_dec([&] (auto&& ...vs) {
            traverse(vs...);
        });
    }

    auto tail_size() const
    {
        return size - tail_offset();
    }

    auto tail_offset() const
    {
        return size ? (size-1) & ~mask<B> : 0;
    }

    template <typename ...Visitors>
    void traverse(Visitors&&... vs) const
    {
        auto tail_off  = tail_offset();
        auto tail_size = size - tail_off;

        if (tail_off) make_regular_rbpos(root, shift, tail_off).visit(vs...);
        else make_empty_regular_rbpos(root).visit(vs...);

        if (tail_size) make_leaf_rbpos(tail, tail_size).visit(vs...);
        else make_empty_leaf_rbpos(tail).visit(vs...);
    }

    template <typename Step, typename State>
    State reduce(Step step, State init) const
    {
        return do_reduce(step, init, [&] (auto&& ...vs) {
                return traverse(*this, vs...);
            });
    }

    const T* array_for(std::size_t index) const
    {
        assert(index < size);

        if (index >= tail_offset())
            return tail->leaf();
        else {
            auto node = root;
            for (auto level = shift; level; level -= B) {
                node = node->inner() [(index >> level) & mask<B>];
            }
            return node->leaf();
        }
    }

    node_t* make_path(unsigned level, node_t* node) const
    {
        return level == 0
            ? node
            : node_t::make_inner(make_path(level - B, std::move(node)));
    }

    node_t* push_tail(unsigned level,
                      node_t* parent,
                      node_t* tail) const
    {
        auto idx        = ((size - branches<B> - 1) >> level) & mask<B>;
        auto new_idx    = ((size - 1) >> level) & mask<B>;
        auto new_parent = node_t::copy_inner(parent, new_idx);
        new_parent->inner()[new_idx] =
            level == B       ? tail :
            idx == new_idx   ? push_tail(level - B, parent->inner()[idx], tail)
            /* otherwise */  : make_path(level - B, tail);
        return new_parent;
    }

    rbtree push_back(T value) const
    {
        auto ts = tail_size();
        if (ts < branches<B>) {
            auto new_tail = node_t::copy_leaf_emplace(tail, ts,
                                                      std::move(value));
            return { size + 1, shift, root->inc(), new_tail };
        } else {
            auto new_tail = node_t::make_leaf(std::move(value));
            if ((size >> B) > (1u << shift)) {
                auto new_root = node_t::make_inner(
                    root->inc(),
                    make_path(shift, tail->inc()));
                return { size + 1, shift + B, new_root, new_tail };
            } else {
                auto new_root = push_tail(shift, root, tail->inc());
                return { size + 1, shift, new_root, new_tail };
            }
        }
    }

    const T& get(std::size_t index) const
    {
        auto arr = array_for(index);
        return arr [index & mask<B>];
    }

    template <typename FnT>
    rbtree update(std::size_t idx, FnT&& fn) const
    {
        auto tail_off = tail_offset();
        if (idx >= tail_off) {
            auto new_tail  = node_t::copy_leaf(tail, size - tail_off);
            auto& item     = new_tail->leaf() [idx & mask<B>];
            auto new_value = std::forward<FnT>(fn) (std::move(item));
            item = std::move(new_value);
            return { size, shift, root->inc(), new_tail };
        } else {
            auto new_root = do_update_last(shift,
                                           root,
                                           idx,
                                           std::forward<FnT>(fn));
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
