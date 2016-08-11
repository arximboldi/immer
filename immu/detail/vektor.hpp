
#pragma once

#include <immu/detail/node.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <memory>
#include <cassert>

namespace immu {
namespace detail {
namespace vektor {

template <typename T, int B>
struct impl
{
    using inner_t    = inner_node<T, B>;
    using leaf_t     = leaf_node<T, B>;
    using node_t     = node<T, B>;
    using node_ptr_t = node_ptr<T, B>;

    std::size_t size;
    unsigned    shift;
    node_ptr_t  root;
    node_ptr_t  tail;

    auto tail_offset() const
    {
        return size < branches<B> ? 0 : ((size - 1) >> B) << B;
    }

    template <typename ...Ts>
    static auto make_node(Ts&& ...xs)
    {
        return detail::make_node<T, B>(std::forward<Ts>(xs)...);
    }

    template <typename Step, typename State>
    void do_reduce_leaf(Step&& fn, State& acc, const node_t& node) const
    {
        for (auto&& x : node.leaf())
            acc = fn(std::move(acc), x);
    }

    template <typename Step, typename State>
    void do_reduce_node_full(Step&& fn, State& acc, const node_t& node, unsigned level) const
    {
        assert(level > 0);
        auto next = level - B;
        if (next == 0) {
            for (auto&& n : node.inner())
                do_reduce_leaf(fn, acc, *n);
        } else {
            for (auto&& n : node.inner())
                do_reduce_node_full(fn, acc, *n, next);
        }
    }

    template <typename Step, typename State>
    void do_reduce_node_last(Step&& fn, State& acc, const node_t& node, unsigned level) const
    {
        assert(level > 0);
        auto next = level - B;
        auto last = (size >> level) & mask<B>;
        auto iter = node.inner().begin();
        if (next == 0) {
            auto end = iter + last;
            for (; iter != end; ++iter)
                do_reduce_leaf(fn, acc, **iter);
        } else {
            auto end = iter + last - 1;
            for (; iter != end; ++iter)
                do_reduce_node_full(fn, acc, **iter, next);
            do_reduce_node_last(fn, acc, **iter, next);
        }
    }

    template <typename Step, typename State>
    void do_reduce_tail(Step&& fn, State& acc) const
    {
        auto tail_size = size - tail_offset();
        auto iter = tail->leaf().begin();
        auto end  = iter + tail_size;
        for (; iter != end; ++iter)
            acc = fn(acc, *iter);
    }

    template <typename Step, typename State>
    State reduce(Step fn, State acc) const
    {
        if (size > branches<B>)
            do_reduce_node_last(fn, acc, *root, shift);
        do_reduce_tail(fn, acc);
        return acc;
    }

    const leaf_t& array_for(std::size_t index) const
    {
        assert(index < size);

        if (index >= tail_offset())
            return tail->leaf();
        else {
            auto node = root.get();
            for (auto level = shift; level; level -= B) {
                node = node->inner() [(index >> level) & mask<B>].get();
            }
            return node->leaf();
        }
    }

    node_ptr_t make_path(unsigned level, node_ptr_t node) const
    {
        return level == 0
            ? node
            : make_node(inner_t{{ make_path(level - B,
                                            std::move(node)) }});
    }

    node_ptr_t push_tail(unsigned level,
                         const node_t& parent_,
                         node_ptr_t tail) const
    {
        const auto& parent   = parent_.inner();
        auto new_parent_node = make_node(parent);
        auto& new_parent     = new_parent_node->inner();
        auto idx             = ((size - 1) >> level) & mask<B>;
        auto next_node =
            level == B ? std::move(tail) :
            parent[idx]            ? push_tail(level - B,
                                               *parent[idx],
                                               std::move(tail)) :
            /* otherwise */          make_path(level - B,
                                               std::move(tail));
        new_parent[idx] = next_node;
        return new_parent_node;
    }

    impl push_back(T value) const
    {
        auto tail_size = size - tail_offset();
        if (tail_size < branches<B>) {
            const auto& old_tail = tail->leaf();
            auto  new_tail_node  = make_node(leaf_t{});
            auto& new_tail       = new_tail_node->leaf();
            std::copy(old_tail.begin(),
                      old_tail.begin() + tail_size,
                      new_tail.begin());
            new_tail[tail_size] = std::move(value);
            return impl{ size + 1,
                    shift,
                    root,
                    std::move(new_tail_node) };
        } else {
            auto new_tail_node = make_node(leaf_t {{ std::move(value) }});
            return ((size >> B) > (1u << shift))
                ? impl{ size + 1,
                    shift + B,
                    make_node(inner_t{{ root, make_path(shift, tail) }}),
                    new_tail_node }
            : impl{ size + 1,
                      shift,
                      push_tail(shift, *root, tail),
                      new_tail_node };
        }
    }

    const T& get(std::size_t index) const
    {
        const auto& arr = array_for(index);
        return arr [index & mask<B>];
    }

    template <typename FnT>
    impl update(std::size_t idx, FnT&& fn) const
    {
        if (idx >= tail_offset()) {
            const auto& old_tail = tail->leaf();
            auto new_tail_node  = make_node(leaf_t{old_tail});
            auto& item = new_tail_node->leaf() [idx & mask<B>];
            auto new_value = std::forward<FnT>(fn) (std::move(item));
            item = std::move(new_value);
            return impl{ size,
                    shift,
                    root,
                    std::move(new_tail_node) };
        } else {
            return impl{size,
                    shift,
                    do_update(shift,
                              *root,
                              idx,
                              std::forward<FnT>(fn)),
                    tail};
        }
    }

    template <typename FnT>
    node_ptr_t do_update(unsigned level,
                         const node_t& node,
                         std::size_t idx,
                         FnT&& fn) const
    {
        if (level == 0) {
            auto new_node  = make_node(leaf_t{node.leaf()});
            auto& item     = new_node->leaf() [idx & mask<B>];
            auto new_value = std::forward<FnT>(fn) (std::move(item));
            item = std::move(new_value);
            return new_node;
        } else {
            auto new_node = make_node(inner_t{node.inner()});
            auto& item    = new_node->inner()[(idx >> level) & mask<B>];
            item = do_update(level - B, *item, idx, std::forward<FnT>(fn));
            return new_node;
        }
    }

    impl assoc(std::size_t idx, T value) const
    {
        return update(idx, [&] (auto&&) {
                return std::move(value);
            });
    }
};

template <typename T, int B> const auto    empty_inner = make_node<T, B>(inner_node<T, B>{});
template <typename T, int B> const auto    empty_leaf  = make_node<T, B>(leaf_node<T, B>{});
template <typename T, int B> const impl<T, B> empty       = {
    0,
    B,
    empty_inner<T, B>,
    empty_leaf<T, B>
};

template <typename T, int B>
struct iterator : boost::iterator_facade<
    iterator<T, B>,
    T,
    boost::random_access_traversal_tag,
    const T&>
{
    struct end_t {};

    iterator() = default;

    iterator(const impl<T, B>& v)
        : v_    { &v }
        , i_    { 0 }
        , base_ { 0 }
        , curr_ { v.array_for(0).begin() }
    {
    }

    iterator(const impl<T, B>& v, end_t)
        : v_    { &v }
        , i_    { v.size }
        , base_ { i_ - (i_ & mask<B>) }
        , curr_ { v.array_for(i_ - 1).begin() + (i_ - base_) }
    {}

private:
    friend class boost::iterator_core_access;

    using leaf_iterator = typename leaf_node<T, B>::const_iterator;

    const impl<T, B>* v_;
    std::size_t       i_;
    std::size_t       base_;
    leaf_iterator     curr_;

    void increment()
    {
        assert(i_ < v_->size);
        ++i_;
        if (i_ - base_ < branches<B>) {
            ++curr_;
        } else {
            base_ += branches<B>;
            curr_ = v_->array_for(i_).begin();
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
            curr_ = std::prev(v_->array_for(i_).end());
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
            curr_ = v_->array_for(i_).begin() + (i_ - base_);
        }
    }

    bool equal(const iterator& other) const
    {
        return i_ == other.i_;
    }

    std::ptrdiff_t distance_to(const iterator& other) const
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

} /* namespace vektor */
} /* namespace detail */
} /* namespace immu */
