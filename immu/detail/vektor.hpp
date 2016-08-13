
#pragma once

#include <immu/detail/free_list.hpp>
#include <immu/detail/ref_count_base.hpp>

#include <boost/iterator/iterator_facade.hpp>
#include <boost/iterator/iterator_facade.hpp>

#include <cassert>
#include <memory>

namespace immu {
namespace detail {
namespace vektor {

template <int B, typename T=std::size_t>
constexpr T branches = T{1} << B;

template <int B, typename T=std::size_t>
constexpr T mask = branches<B, T> - 1;

template <typename T, int B>
struct node;

template <typename T, int B>
using node_ptr = node<T, B>*;

template <typename T, int B>
using leaf_node  = std::array<T, 1 << B>;

template <typename T, int B>
using inner_node = std::array<node_ptr<T, B>, 1 << B>;

#ifdef NDEBUG
#define IMMU_TAGGED_NODES 0
#else
#define IMMU_TAGGED_NODES 1
#endif

template <typename T, int B>
struct node_base
{
    using leaf_node_t  = leaf_node<T, B>;
    using inner_node_t = inner_node<T, B>;

    mutable std::atomic<int> ref_count {1};

    union data_t
    {
        leaf_node_t  leaf;
        inner_node_t inner;
        data_t(leaf_node_t n)  : leaf(std::move(n)) {}
        data_t(inner_node_t n) : inner(std::move(n)) {}
        ~data_t() {}
    } data;

#if IMMU_TAGGED_NODES
    enum
    {
        leaf_kind,
        inner_kind
    } kind;
#endif // IMMU_TAGGED_NODES

    node_base(leaf_node<T, B> n)
        : data{std::move(n)}
#if IMMU_TAGGED_NODES
        , kind{leaf_kind}
#endif // IMMU_TAGGED_NODES
    {}

    node_base(inner_node<T, B> n)
        : data{std::move(n)}
#ifndef NDEBUG
        , kind{inner_kind}
#endif // IMMU_TAGGED_NODES
    {}

    inner_node_t& inner() & {
        assert(kind == inner_kind);
        return data.inner;
    }
    const inner_node_t& inner() const& {
        assert(kind == inner_kind);
        return data.inner;
    }
    inner_node_t&& inner() && {
        assert(kind == inner_kind);
        return std::move(data.inner);
    }

    leaf_node_t& leaf() & {
        assert(kind == leaf_kind);
        return data.leaf;
    }
    const leaf_node_t& leaf() const& {
        assert(kind == leaf_kind);
        return data.leaf;
    }
    leaf_node_t&& leaf() && {
        assert(kind == leaf_kind);
        return std::move(data.leaf);
    }
};

template <typename T, int B>
struct node : node_base<T, B>
            , with_thread_local_free_list<sizeof(node_base<T, B>)>
{
    using node_base<T, B>::node_base;
};

template <typename T, int B, typename ...Ts>
auto make_node(Ts&& ...xs) -> node_ptr<T, B>
{
    return new node<T, B>(std::forward<Ts>(xs)...);
}

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

    static const impl empty;

    impl(std::size_t sz, unsigned sh, node_ptr_t r, node_ptr_t t)
        : size{sz}, shift{sh}, root{r}, tail{t}
    {
        assert(r->ref_count.load() > 0);
        assert(t->ref_count.load() > 0);
    }

    impl(const impl& other)
        : impl{other.size, other.shift, other.root, other.tail}
    {
        inc();
    }

    impl(impl&& other)
        : impl{empty}
    {
        swap(*this, other);
    }

    impl& operator=(const impl& other)
    {
        auto next = other;
        swap(*this, other);
        return *this;
    }

    impl& operator=(impl&& other)
    {
        swap(*this, other);
        return *this;
    }

    friend void swap(impl& x, impl& y)
    {
        using std::swap;
        swap(x.size,  y.size);
        swap(x.shift, y.shift);
        swap(x.root,  y.root);
        swap(x.tail,  y.tail);
    }

    ~impl() {
        dec();
    }

    void inc_node(const node_ptr_t n) const
    {
        n->ref_count.fetch_add(1, std::memory_order_relaxed);
    }

    void inc() const
    {
        inc_node(root);
        inc_node(tail);
    }

    void dec_leaf(node_ptr_t n) const
    {
        if (n->ref_count.fetch_sub(1, std::memory_order_release) == 1) {
            std::atomic_thread_fence(std::memory_order_acquire);
            delete n;
        }
    }

    void dec_inner_full(node_ptr_t n, unsigned level) const
    {
        if (n->ref_count.fetch_sub(1, std::memory_order_release) == 1) {
            std::atomic_thread_fence(std::memory_order_acquire);
            auto next = level - B;
            if (next == 0) {
                for (auto&& n : n->inner())
                    dec_leaf(n);
            } else {
                for (auto&& n : n->inner())
                    dec_inner_full(n, next);
            }
            delete n;
        }
    }

    void dec_inner_last(node_ptr_t n, unsigned level) const
    {
        if (n->ref_count.fetch_sub(1, std::memory_order_release) == 1) {
            std::atomic_thread_fence(std::memory_order_acquire);
            auto next = level - B;
            auto last = ((tail_offset()-1) >> level) & mask<B>;
            auto iter = n->inner().begin();
            if (next == 0) {
                auto end = iter + last + 1;
                for (; iter != end; ++iter)
                    dec_leaf(*iter);
            } else {
                auto end = iter + last;
                for (; iter != end; ++iter)
                    dec_inner_full(*iter, next);
                dec_inner_last(*iter, next);
            }
            delete n;
        }
    }

    void dec() const
    {
        if (size > branches<B>)
            dec_inner_last(root, shift);
        else {
            if (root->ref_count.fetch_sub(1, std::memory_order_release) == 1) {
                std::atomic_thread_fence(std::memory_order_acquire);
                delete root;
            }
        }
        dec_leaf(tail);
    }

    auto tail_offset() const
    {
        return size < branches<B> ? 0 : ((size - 1) >> B) << B;
    }

    template <typename ...Ts>
    static auto make_node(Ts&& ...xs)
    {
        return vektor::make_node<T, B>(std::forward<Ts>(xs)...);
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
        auto last = ((tail_offset()-1) >> level) & mask<B>;
        auto iter = node.inner().begin();
        if (next == 0) {
            auto end = iter + last + 1;
            for (; iter != end; ++iter)
                do_reduce_leaf(fn, acc, **iter);
        } else {
            auto end = iter + last;
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
            auto node = root;
            for (auto level = shift; level; level -= B) {
                node = node->inner() [(index >> level) & mask<B>];
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
        auto idx             = ((size - branches<B> - 1) >> level) & mask<B>;
        auto new_idx         = ((size - 1) >> level) & mask<B>;
        const auto& parent   = parent_.inner();
        auto new_parent_node = make_node(parent);
        auto& new_parent     = new_parent_node->inner();
        for (auto iter = new_parent.begin(), last = new_parent.begin() + new_idx;
             iter != last; ++iter)
            inc_node(*iter);
        auto next_node =
            level == B       ? tail :
            idx == new_idx   ? push_tail(level - B,
                                         *parent[idx],
                                         tail) :
            /* otherwise */    make_path(level - B,
                                         tail);
        new_parent[new_idx] = next_node;
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
            inc_node(root);
            return impl{ size + 1,
                    shift,
                    root,
                    std::move(new_tail_node) };
        } else {
            auto new_tail_node = make_node(leaf_t {{ std::move(value) }});
            if ((size >> B) > (1u << shift)) {
                inc_node(root);
                inc_node(tail);
                return impl{ size + 1,
                        shift + B,
                        make_node(inner_t{{ root, make_path(shift, tail) }}),
                        new_tail_node };
            } else {
                inc_node(tail);
                return impl{ size + 1,
                        shift,
                        push_tail(shift, *root, tail),
                        new_tail_node };
            }
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
            inc_node(root);
            return impl{ size,
                    shift,
                    root,
                    std::move(new_tail_node) };
        } else {
            inc_node(tail);
            return impl{size,
                    shift,
                    do_update_last(shift,
                                   *root,
                                   idx,
                                   std::forward<FnT>(fn)),
                    tail};
        }
    }

    template <typename FnT>
    node_ptr_t do_update_full(unsigned level,
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
            auto offset = (idx >> level) & mask<B>;
            auto new_node = make_node(inner_t{node.inner()});
            {
                auto iter = new_node->inner().begin();
                for (auto last = iter + offset; iter != last; ++iter)
                    inc_node(*iter);
                ++iter;
                for (auto last = new_node->inner().end(); iter != last; ++iter)
                    inc_node(*iter);
            }
            auto& item    = new_node->inner()[offset];
            item = do_update_full(level - B, *item, idx, std::forward<FnT>(fn));
            return new_node;
        }
    }

    template <typename FnT>
    node_ptr_t do_update_last(unsigned level,
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
            auto offset = (idx >> level) & mask<B>;
            auto end_offset = ((tail_offset()-1) >> level) & mask<B>;
            auto new_node = make_node(inner_t{node.inner()});
            {
                auto iter = new_node->inner().begin();
                for (auto last = iter + offset; iter != last; ++iter)
                    inc_node(*iter);
                ++iter;
                for (auto last = new_node->inner().begin() + end_offset+1; iter != last; ++iter)
                    inc_node(*iter);
            }
            auto& item    = new_node->inner()[offset];
            if (offset == end_offset)
                item = do_update_last(level - B, *item, idx, std::forward<FnT>(fn));
            else
                item = do_update_full(level - B, *item, idx, std::forward<FnT>(fn));
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

template <typename T, int B>
const impl<T, B> impl<T, B>::empty = {
    0,
    B,
    make_node(inner_node<T, B>{}),
    make_node(leaf_node<T, B>{})
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
