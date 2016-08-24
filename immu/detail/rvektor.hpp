
#pragma once

#include <boost/iterator/iterator_facade.hpp>
#include <boost/iterator/iterator_facade.hpp>

#include <cassert>
#include <memory>
#include <numeric>

namespace immu {
namespace detail {
namespace rvektor {

#ifdef NDEBUG
#define IMMU_TAGGED_NODES 0
#else
#define IMMU_TAGGED_NODES 1
#endif

template <int B, typename T=std::size_t>
constexpr T branches = T{1} << B;

template <int B, typename T=std::size_t>
constexpr T mask = branches<B, T> - 1;

template <typename T,
          int B,
          typename MemoryPolicy>
struct impl
{
    using heap_policy = typename MemoryPolicy::heap;
    using refcount    = typename MemoryPolicy::refcount;

    struct node_t : refcount::data
    {
#if IMMU_TAGGED_NODES
        enum
        {
            leaf_kind,
            inner_kind
        } kind;
#endif // IMMU_TAGGED_NODES
        union data_t
        {
            using  leaf_t = T[branches<B>];
            struct inner_t
            {
                node_t* elems[branches<B>];
                std::size_t* sizes;
            };

            inner_t inner;
            leaf_t  leaf;

            data_t() {}
            ~data_t() {}
        } data;

        std::size_t* sizes()
        {
            assert(kind == inner_kind);
            return data.inner.sizes;
        }

        const std::size_t* sizes() const
        {
            assert(kind == inner_kind);
            return data.inner.sizes;
        }

        node_t** inner()
        {
            assert(kind == inner_kind);
            return data.inner.elems;
        }

        const node_t* const* inner() const
        {
            assert(kind == inner_kind);
            return data.inner.elems;
        }

        T* leaf()
        {
            assert(kind == leaf_kind);
            return data.leaf;
        }

        const T* leaf() const
        {
            assert(kind == leaf_kind);
            return data.leaf;
        }
    };

    using heap = typename heap_policy::template apply<
        sizeof(node_t)
    >::type;

    std::size_t size;
    unsigned    shift;
    node_t*     root;
    node_t*     tail;

    static const impl empty;

    static node_t* make_inner()
    {
        auto p = new (heap::allocate(sizeof(node_t))) node_t;
        p->data.inner.sizes = nullptr;
#if IMMU_TAGGED_NODES
        p->kind = node_t::inner_kind;
#endif
        return p;
    }

    static node_t* make_inner_r()
    {
        auto p = new (heap::allocate(sizeof(node_t))) node_t;
        auto s = heap::allocate(sizeof(std::size_t) * branches<B> + 1);
        p->data.inner.sizes = static_cast<std::size_t*>(s);
#if IMMU_TAGGED_NODES
        p->kind = node_t::inner_kind;
#endif
        return p;
    }

    static node_t* make_inner(node_t* x)
    {
        auto p = make_inner();
        p->inner() [0] = x;
        return p;
    }

    static node_t* make_inner(node_t* x, node_t* y)
    {
        auto p = make_inner();
        p->inner() [0] = x;
        p->inner() [1] = y;
        return p;
    }

    static node_t* make_inner(node_t* x, std::size_t xs, node_t* y)
    {
        auto p = make_inner_r();
        p->inner() [0] = x;
        p->inner() [1] = y;
        p->sizes() [0] = xs;
        p->sizes() [1] = xs + branches<B>;
        p->sizes() [branches<B>] = 2;
        return p;
    }

    static node_t* make_leaf()
    {
        auto p = new (heap::allocate(sizeof(node_t))) node_t;
#if IMMU_TAGGED_NODES
        p->kind = node_t::leaf_kind;
#endif
        return p;
    }

    template <typename U>
    static node_t* make_leaf(U&& x)
    {
        auto p = make_leaf();
        new (p->leaf()) T{ std::forward<U>(x) };
        return p;
    }

    static node_t* copy_inner(node_t* src, int n)
    {
        assert(src->kind == node_t::inner_kind);
        auto dst = make_inner();
        for (auto i = src->inner(), e = i + n; i != e; ++i)
            refcount::inc(*i);
        std::uninitialized_copy(src->inner(), src->inner() + n, dst->inner());
        return dst;
    }

    static node_t* copy_inner_r(node_t* src, int n)
    {
        assert(src->kind == node_t::inner_kind);
        auto dst = make_inner();
        for (auto i = src->inner(), e = i + n; i != e; ++i)
            refcount::inc(*i);
        std::uninitialized_copy(src->inner(), src->inner() + n, dst->inner());
        std::uninitialized_copy(src->sizes(), src->sizes() + n, dst->sizes());
        dst->sizes() [branches<B>] = src->sizes() [branches<B>];
        return dst;
    }

    static node_t* copy_leaf(node_t* src, int n)
    {
        assert(src->kind == node_t::leaf_kind);
        auto dst = make_leaf();
        std::uninitialized_copy(src->leaf(), src->leaf() + n, dst->leaf());
        return dst;
    }

    static node_t* copy_leaf(node_t* src1, int n1,
                             node_t* src2, int n2)
    {
        assert(src1->kind == node_t::leaf_kind);
        assert(src2->kind == node_t::leaf_kind);
        auto dst = make_leaf();
        std::uninitialized_copy(
            src1->leaf(), src1->leaf() + n1, dst->leaf());
        std::uninitialized_copy(
            src2->leaf(), src2->leaf() + n2, dst->leaf() + n1);
        return dst;
    }

    static node_t* copy_leaf(node_t* src, int idx, int n)
    {
        assert(src->kind == node_t::leaf_kind);
        auto dst = make_leaf();
        std::uninitialized_copy(
            src->leaf() + idx, src->leaf() + idx + n, dst->leaf());
        return dst;
    }

    template <typename U>
    static node_t* copy_leaf_emplace(node_t* src, int n, U&& x)
    {
        auto dst = copy_leaf(src, n);
        new (dst->leaf() + n) T{std::forward<U>(x)};
        return dst;
    }

    static void delete_inner(node_t* p)
    {
        assert(p->kind == node_t::inner_kind);
        auto sizes = p->sizes();
        if (sizes) heap::deallocate(sizes);
        heap::deallocate(p);
    }

    static void delete_leaf(node_t* p, int n)
    {
        assert(p->kind == node_t::leaf_kind);
        for (auto i = p->leaf(), e = i + n; i != e; ++i)
            i->~T();
        heap::deallocate(p);
    }

    impl(std::size_t sz, unsigned sh, node_t* r, node_t* t)
        : size{sz}, shift{sh}, root{r}, tail{t}
    {}

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
        swap(*this, next);
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

    void inc() const
    {
        refcount::inc(root);
        refcount::inc(tail);
    }

    struct dec_traversal
    {
        bool predicate(node_t* p)
        { return refcount::dec(p); }

        void visit_inner(node_t* p)
        { delete_inner(p); }

        void visit_leaf(node_t* p, unsigned n)
        { delete_leaf(p, n); }
    };

    void dec() const
    {
        traverse(dec_traversal{});
    }

    auto tail_size() const
    {
        return size - tail_offset();
    }

    auto tail_offset() const
    {
        return size ? (size-1) & ~mask<B> : 0;
    }

    template <typename Step, typename State>
    struct reduce_traversal
    {
        Step fn;
        State acc;

        bool predicate(node_t*) { return true; }
        void visit_inner(node_t* n) {}

        void visit_leaf(node_t* n, unsigned elems)
        {
            acc = std::accumulate(n->leaf(),
                                  n->leaf() + elems,
                                  acc,
                                  fn);
        }
    };

    template <typename Step, typename State>
    State reduce(Step step, State init) const
    {
        auto t = reduce_traversal<Step, State>{step, init};
        traverse(t);
        return t.acc;
    }

    template <typename Traversal>
    void traverse_node_last(Traversal&& t, node_t* node, unsigned level) const
    {
        assert(level > 0);
        assert(size > branches<B>);
        if (t.predicate(node)) {
            auto next = level - B;
            auto last = ((tail_offset()-1) >> level) & mask<B>;
            if (next == 0) {
                for (auto i = node->inner(), e = i + last + 1; i != e; ++i)
                    if (t.predicate(*i))
                        t.visit_leaf(*i, branches<B>);
            } else {
                auto i = node->inner();
                for (auto e = i + last; i != e; ++i)
                    traverse_node_full(t, *i, next);
                traverse_node_last(t, *i, next);
            }
            t.visit_inner(node);
        }
    }

    template <typename Traversal>
    void traverse_node_full(Traversal&& t, node_t* node, unsigned level) const
    {
        assert(level > 0);
        assert(size > branches<B>);
        if (t.predicate(node)) {
            auto next = level - B;
            if (next == 0) {
                for (auto i = node->inner(), e = i + branches<B>; i != e; ++i)
                    if (t.predicate(*i))
                        t.visit_leaf(*i, branches<B>);
            } else {
                for (auto i = node->inner(), e = i + branches<B>; i != e; ++i)
                    traverse_node_full(t, *i, next);
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

    const T* array_for(std::size_t& index) const
    {
        assert(index < size);

        if (index >= tail_offset())
            return tail->leaf();
        else {
            auto node = root;
            for (auto level = shift; level; level -= B) {
                auto sizes = node->sizes();
                if (sizes) {
                    auto node_index = index >> level;
                    while (sizes[node_index] <= index) ++node_index;
                    if (node_index) index -= sizes[node_index];
                    node = node->inner() [node_index];
                } else {
                    // balanced subtree
                    do {
                        node = node->inner() [(index >> level) & mask<B>];
                    } while (level -= B);
                    break;
                }
            }
            return node->leaf();
        }
    }

    node_t* make_path(unsigned level, node_t* node) const
    {
        assert(node->kind == node_t::leaf_kind);
        return level == 0
            ? node
            : make_inner(make_path(level - B, std::move(node)));
    }

    node_t* push_tail(unsigned level,
                      node_t* parent,
                      node_t* tail) const
    {
        auto sizes = parent->sizes();
        if (sizes) {
            auto idx = sizes[branches<B>];
            auto children   = idx ? sizes[idx] - sizes[idx - 1] : sizes[idx];
            auto new_idx    = children == 1 << level ? idx + 1 : idx;
            auto new_parent = copy_inner_r(parent, new_idx);
            new_parent->inner()[new_idx] =
                level == B       ? tail :
                idx == new_idx   ? push_tail(level - B, parent->inner()[idx], tail)
                /* otherwise */  : make_path(level - B, tail);
            new_parent->sizes()[new_idx] = sizes[idx] + branches<B>;
            new_parent->sizes()[branches<B>] = sizes[branches<B>] + (idx != new_idx);
            return new_parent;
        } else {
            auto idx        = ((size - branches<B> - 1) >> level) & mask<B>;
            auto new_idx    = ((size - 1) >> level) & mask<B>;
            auto new_parent = copy_inner(parent, new_idx);
            new_parent->inner()[new_idx] =
                level == B       ? tail :
                idx == new_idx   ? push_tail(level - B, parent->inner()[idx], tail)
                /* otherwise */  : make_path(level - B, tail);
            return new_parent;
        }
    }

    bool is_overflow() const
    {
        auto node  = root;
        auto sizes = node->sizes();
        auto count = size;
        if (!sizes)
            return (count >> B) > (1u << shift);
        for (auto level = shift; level > B; level -= B) {
            if (!sizes)
                return (count >> B) > (1u << level);
            else {
                auto s = sizes[branches<B>];
                if (s != branches<B>)
                    return false;
                node  = node->inner() [s - 1];
                count = (sizes[branches<B> - 1]  -
                         sizes[branches<B> - 2]) + branches<B>;
            }
        }
        return true;
    }

    std::pair<unsigned, node_t*> push_tail_into_root(node_t* tail) const
    {
        if (is_overflow()) {
            refcount::inc(root);
            auto new_path = make_path(shift, tail);
            auto new_root = root->sizes()
                ? make_inner(root,
                             root->sizes() [branches<B> - 1],
                             new_path)
                : make_inner(root, new_path);
            return { shift + B, new_root };
        } else {
            auto new_root = push_tail(shift, root, tail);
            return { shift, new_root };
        }
    }

    impl push_back(T value) const
    {
        auto ts = tail_size();
        if (ts < branches<B>) {
            auto new_tail = copy_leaf_emplace(tail, ts, std::move(value));
            refcount::inc(root);
            return { size + 1, shift, root, new_tail };
        } else {
            auto new_tail = make_leaf(std::move(value));
            auto new_root = push_tail_into_root(tail);
            refcount::inc(tail);
            return { size + 1, new_root.first, new_root.second, new_tail };
        }
    }

    const T& get(std::size_t index) const
    {
        auto arr = array_for(index);
        return arr [index & mask<B>];
    }

    template <typename FnT>
    impl update(std::size_t idx, FnT&& fn) const
    {
        auto tail_off = tail_offset();
        if (idx >= tail_off) {
            auto new_tail  = copy_leaf(tail, size - tail_off);
            auto& item     = new_tail->leaf() [idx & mask<B>];
            auto new_value = std::forward<FnT>(fn) (std::move(item));
            item = std::move(new_value);
            refcount::inc(root);
            return { size, shift, root, new_tail };
        } else {
            refcount::inc(tail);
            auto new_root = do_update(shift,
                                      root,
                                      idx,
                                      tail_offset(),
                                      std::forward<FnT>(fn));
            return { size, shift, new_root, tail};
        }
    }

    template <typename FnT>
    node_t* do_update(unsigned level,
                      node_t* node,
                      std::size_t idx,
                      std::size_t count,
                      FnT&& fn) const
    {
        auto sizes = node->sizes();
        if (level == 0) {
            auto new_node  = copy_leaf(node, branches<B>);
            auto& item     = new_node->leaf() [idx & mask<B>];
            auto new_value = std::forward<FnT>(fn) (std::move(item));
            item = std::move(new_value);
            return new_node;
        } else if (sizes) {
            auto offset   = (idx >> level) & mask<B>;
            while (sizes[offset] <= idx) ++offset;
            auto new_node = copy_inner_r(node, sizes[branches<B>]);
            refcount::dec_unsafe(node->inner()[offset]);
            new_node->inner()[offset] =
                do_update(level - B,
                          node->inner()[offset],
                          offset ? idx - sizes[idx] : idx,
                          sizes[idx],
                          std::forward<FnT>(fn));
            return new_node;
        } else {
            return do_update_last(level, node, idx, count,
                                  std::forward<FnT>(fn));
        }
    }

    template <typename FnT>
    node_t* do_update_full(unsigned level,
                           node_t* node,
                           std::size_t idx,
                           FnT&& fn) const
    {
        if (level == 0) {
            auto new_node  = copy_leaf(node, branches<B>);
            auto& item     = new_node->leaf() [idx & mask<B>];
            auto new_value = std::forward<FnT>(fn) (std::move(item));
            item = std::move(new_value);
            return new_node;
        } else {
            auto offset   = (idx >> level) & mask<B>;
            auto new_node = copy_inner(node, branches<B>);
            refcount::dec_unsafe(node->inner()[offset]);
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
                           std::size_t count,
                           FnT&& fn) const
    {
        if (level == 0) {
            auto new_node  = copy_leaf(node, branches<B>);
            auto& item     = new_node->leaf() [idx & mask<B>];
            auto new_value = std::forward<FnT>(fn) (std::move(item));
            item = std::move(new_value);
            return new_node;
        } else {
            auto offset     = (idx >> level) & mask<B>;
            auto end_offset = ((count - 1) >> level) & mask<B>;
            auto new_node   = copy_inner(node, end_offset + 1);
            refcount::dec_unsafe(node->inner()[offset]);
            new_node->inner()[offset] =
                offset == end_offset
                ? do_update_last(level - B, node->inner()[offset], idx, count,
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

    impl concat(const impl& r) const
    {
        if (size == 0)
            return r;
        else if (r.size == 0)
            return *this;
        else if (r.size <= branches<B>) {
            // just concat the tail, similar to push_back
            auto ts = tail_size();
            if (ts == branches<B>) {
                auto new_tail = r.tail;
                refcount::inc(new_tail);
                auto new_root = push_tail_into_root(tail);
                refcount::inc(tail);
                return { size + r.size, new_root.first, new_root.second, new_tail };
            } else if (ts + r.size <= branches<B>) {
                auto new_tail = copy_leaf(tail, ts, r.tail, r.size);
                refcount::inc(root);
                return { size + r.size, shift, root, new_tail };
            } else {
                auto remaining = branches<B> - ts;
                auto add_tail  = copy_leaf(tail, ts, r.tail, remaining);
                auto new_tail  = copy_leaf(r.tail, remaining, r.size);
                auto new_root = push_tail_into_root(add_tail);
                return { size + r.size, new_root.first, new_root.second, new_tail };
            }
        } else {
            return *this;
        }
    }
};

template <typename T, int B, typename MP>
const impl<T, B, MP> impl<T, B, MP>::empty = {
    0,
    B,
    make_inner(),
    make_leaf()
};

template <typename T, int B, typename MP>
struct iterator : boost::iterator_facade<
    iterator<T, B, MP>,
    T,
    boost::random_access_traversal_tag,
    const T&>
{
    struct end_t {};

    iterator() = default;

    iterator(const impl<T, B, MP>& v)
        : v_    { &v }
        , i_    { 0 }
        , base_ { 0 }
          //, curr_ { v.array_for(0) } // WIP
    {
    }

    iterator(const impl<T, B, MP>& v, end_t)
        : v_    { &v }
        , i_    { v.size }
        , base_ { i_ - (i_ & mask<B>) }
          //, curr_ { v.array_for(i_ - 1) + (i_ - base_) } // WIP
    {}

private:
    friend class boost::iterator_core_access;

    const impl<T, B, MP>* v_;
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

} /* namespace rvektor */
} /* namespace detail */
} /* namespace immu */
