
#pragma once

#include <immu/detail/free_list.hpp>
#include <immu/detail/ref_count_base.hpp>

#include <boost/iterator/iterator_facade.hpp>
#include <boost/iterator/iterator_facade.hpp>

#include <cassert>
#include <memory>
#include <numeric>

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

    ~impl()
    {
        dec();
    }

    void inc_node(const node_ptr_t n) const
    {
        n->ref_count.fetch_add(1, std::memory_order_relaxed);
    }

    void dec_node_unsafe(const node_ptr_t n) const
    {
        assert(n->ref_count.load() > 1);
        n->ref_count.fetch_sub(1, std::memory_order_relaxed);
    }

    void inc() const
    {
        inc_node(root);
        inc_node(tail);
    }

    void dec() const
    {
        struct traversal {
            bool predicate(node_ptr_t n) {
                return 1 == n->ref_count.fetch_sub(
                    1, std::memory_order_release);
            }
            void visit_inner(node_ptr_t n) {
                std::atomic_thread_fence(std::memory_order_acquire);
                delete n;
            }
            void visit_leaf(node_ptr_t n, unsigned) {
                std::atomic_thread_fence(std::memory_order_acquire);
                delete n;
            }
        };
        traverse(traversal{});
    }

    auto tail_size() const
    {
        return size - tail_offset();
    }

    auto tail_offset() const
    {
        return size ? (size-1) & ~mask<B> : 0;
    }

    template <typename ...Ts>
    static auto make_node(Ts&& ...xs)
    {
        return vektor::make_node<T, B>(std::forward<Ts>(xs)...);
    }

    template <typename Step, typename State>
    State reduce(Step step, State init) const
    {
        struct traversal {
            Step fn;
            State acc;
            bool predicate(node_ptr_t n) { return true; }
            void visit_inner(node_ptr_t n) {}
            void visit_leaf(node_ptr_t n, unsigned elems) {
                acc = std::accumulate(n->leaf().begin(),
                                      n->leaf().begin() + elems,
                                      acc,
                                      fn);
            }
        };
        auto t = traversal{step, init};
        traverse(t);
        return t.acc;
    }

    template <typename Traversal>
    void traverse_node_last(Traversal&& t, node_ptr_t node, unsigned level) const
    {
        assert(level > 0);
        assert(size > branches<B>);
        if (t.predicate(node)) {
            auto next = level - B;
            auto last = ((tail_offset()-1) >> level) & mask<B>;
            if (next == 0) {
                auto iter = node->inner().begin();
                auto end = iter + last + 1;
                for (; iter != end; ++iter)
                    if (t.predicate(*iter))
                        t.visit_leaf(*iter, branches<B>);
            } else {
                auto iter = node->inner().begin();
                auto end = iter + last;
                for (; iter != end; ++iter)
                    traverse_node_full(t, *iter, next);
                traverse_node_last(t, *iter, next);
            }
            t.visit_inner(node);
        }
    }

    template <typename Traversal>
    void traverse_node_full(Traversal&& t, node_ptr_t node, unsigned level) const
    {
        assert(level > 0);
        assert(size > branches<B>);
        if (t.predicate(node)) {
            auto next = level - B;
            if (next == 0) {
                for (auto n : node->inner())
                    if (t.predicate(n))
                        t.visit_leaf(n, branches<B>);
            } else {
                for (auto n : node->inner())
                    traverse_node_full(t, n, next);
            }
            t.visit_inner(node);
        }
    }

    template <typename Traversal>
    void traverse(Traversal&& t) const
    {
        if (size > branches<B>)
            traverse_node_last(t, root, shift);
        else if (t.predicate(root))
            t.visit_inner(root);
        if (t.predicate(tail))
            t.visit_leaf(tail, tail_size());
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
                         node_ptr_t parent_node,
                         node_ptr_t tail) const
    {
        auto idx             = ((size - branches<B> - 1) >> level) & mask<B>;
        auto new_idx         = ((size - 1) >> level) & mask<B>;
        const auto& parent   = parent_node->inner();
        auto new_parent_node = make_node(parent);
        auto iter = parent.begin();
        auto last = iter + new_idx;
        for (; iter != last; ++iter)
            inc_node(*iter);
        new_parent_node->inner()[new_idx] =
            level == B       ? tail :
            idx == new_idx   ? push_tail(level - B, parent[idx], tail) :
            /* otherwise */    make_path(level - B, tail);
        return new_parent_node;
    }

    impl push_back(T value) const
    {
        auto ts = tail_size();
        if (ts < branches<B>) {
            const auto& old_tail = tail->leaf();
            auto  new_tail_node  = make_node(leaf_t{});
            auto& new_tail       = new_tail_node->leaf();
            std::copy(old_tail.begin(),
                      old_tail.begin() + ts,
                      new_tail.begin());
            new_tail[ts] = std::move(value);
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
                        push_tail(shift, root, tail),
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
                                   root,
                                   idx,
                                   std::forward<FnT>(fn)),
                    tail};
        }
    }

    template <typename FnT>
    node_ptr_t do_update_full(unsigned level,
                              node_ptr_t node,
                              std::size_t idx,
                              FnT&& fn) const
    {
        if (level == 0) {
            auto new_node  = make_node(leaf_t{node->leaf()});
            auto& item     = new_node->leaf() [idx & mask<B>];
            auto new_value = std::forward<FnT>(fn) (std::move(item));
            item = std::move(new_value);
            return new_node;
        } else {
            auto offset = (idx >> level) & mask<B>;
            auto new_node = make_node(inner_t{node->inner()});
            for (auto n : node->inner())
                inc_node(n);
            dec_node_unsafe(node->inner()[offset]);
            new_node->inner()[offset] =
                do_update_full(level - B, node->inner()[offset], idx,
                               std::forward<FnT>(fn));
            return new_node;
        }
    }

    template <typename FnT>
    node_ptr_t do_update_last(unsigned level,
                              node_ptr_t node,
                              std::size_t idx,
                              FnT&& fn) const
    {
        if (level == 0) {
            auto new_node  = make_node(leaf_t{node->leaf()});
            auto& item     = new_node->leaf() [idx & mask<B>];
            auto new_value = std::forward<FnT>(fn) (std::move(item));
            item = std::move(new_value);
            return new_node;
        } else {
            auto offset = (idx >> level) & mask<B>;
            auto end_offset = ((tail_offset()-1) >> level) & mask<B>;
            auto new_node = make_node(inner_t{node->inner()});
            auto iter = node->inner().begin();
            auto last = node->inner().begin() + end_offset + 1;
            for (; iter != last; ++iter)
                inc_node(*iter);
            dec_node_unsafe(node->inner()[offset]);
            new_node->inner()[offset] =
                offset == end_offset
                ? do_update_last(level - B, node->inner()[offset], idx,
                                 std::forward<FnT>(fn))
                : do_update_full(level - B, node->inner()[offset], idx,
                                 std::forward<FnT>(fn));
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
