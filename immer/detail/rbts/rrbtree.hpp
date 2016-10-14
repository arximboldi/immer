
#pragma once

#include <immer/config.hpp>
#include <immer/detail/rbts/node.hpp>
#include <immer/detail/rbts/position.hpp>
#include <immer/detail/rbts/algorithm.hpp>

#include <cassert>
#include <memory>
#include <numeric>

namespace immer {
namespace detail {
namespace rbts {

template <typename T,
          int B,
          typename MemoryPolicy>
struct rrbtree
{
    using heap_policy = typename MemoryPolicy::heap;
    using refcount    = typename MemoryPolicy::refcount;

    using node_t = node<T, B, MemoryPolicy>;
    using heap   = typename node_t::heap;

    std::size_t size;
    unsigned    shift;
    node_t*     root;
    node_t*     tail;

    static const rrbtree empty;

    rrbtree(std::size_t sz, unsigned sh, node_t* r, node_t* t)
        : size{sz}, shift{sh}, root{r}, tail{t}
    {
        assert(check_tree());
    }

    rrbtree(const rrbtree& other)
        : rrbtree{other.size, other.shift, other.root, other.tail}
    {
        inc();
    }

    rrbtree(rrbtree&& other)
        : rrbtree{empty}
    {
        swap(*this, other);
    }

    rrbtree& operator=(const rrbtree& other)
    {
        auto next{other};
        swap(*this, next);
        return *this;
    }

    rrbtree& operator=(rrbtree&& other)
    {
        swap(*this, other);
        return *this;
    }

    friend void swap(rrbtree& x, rrbtree& y)
    {
        using std::swap;
        swap(x.size,  y.size);
        swap(x.shift, y.shift);
        swap(x.root,  y.root);
        swap(x.tail,  y.tail);
    }

    ~rrbtree()
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

    static void dec_node(node_t* n, unsigned shift, std::size_t size)
    {
        visit_maybe_relaxed_sub(n, shift, size, dec_visitor());
    }

    static void dec_node(node_t* n, unsigned shift)
    {
        assert(n->relaxed());
        make_relaxed_pos(n, shift, n->relaxed()).visit(dec_visitor());
    }

    auto tail_size() const
    {
        return size - tail_offset();
    }

    auto tail_offset() const
    {
        auto r = root->relaxed();
        assert(r == nullptr || r->count);
        return
            r               ? r->sizes[r->count - 1] :
            size            ? (size - 1) & ~mask<B>
            /* otherwise */ : 0;
    }

    template <typename Visitor, typename... Args>
    void traverse(Visitor v, Args&&... args) const
    {
        auto tail_off  = tail_offset();
        auto tail_size = size - tail_off;

        if (tail_off) visit_maybe_relaxed_sub(root, shift, tail_off, v, args...);
        else make_empty_regular_pos(root).visit(v, args...);

        if (tail_size) make_leaf_sub_pos(tail, tail_size).visit(v, args...);
        else make_empty_leaf_pos(tail).visit(v, args...);
    }

    template <typename Visitor>
    decltype(auto) descend(Visitor v, std::size_t idx) const
    {
        auto tail_off  = tail_offset();
        return idx >= tail_off
            ? make_leaf_descent_pos(tail).visit(v, idx - tail_off)
            : visit_maybe_relaxed_descent(root, shift, v, idx);
    }

    template <typename Step, typename State>
    State reduce(Step step, State acc) const
    {
        traverse(reduce_visitor{}, step, acc);
        return acc;
    }

    std::tuple<unsigned, node_t*>
    push_tail(node_t* root, unsigned shift, std::size_t size,
              node_t* tail, unsigned tail_size) const
    {
        if (auto r = root->relaxed()) {
            auto new_root = make_relaxed_pos(root, shift, r)
                .visit(push_tail_visitor<node_t>{}, tail, tail_size);
            if (new_root)
                return { shift, new_root };
            else {
                auto new_path = node_t::make_path(shift, tail);
                auto new_root = node_t::make_inner_r_n(2u,
                                                       root->inc(),
                                                       size,
                                                       new_path,
                                                       tail_size);
                return { shift + B, new_root };
            }
        } else if (size >> B >= 1u << shift) {
            auto new_path = node_t::make_path(shift, tail);
            auto new_root = node_t::make_inner_n(2u, root->inc(), new_path);
            return { shift + B, new_root };
        } else if (size) {
            auto new_root = make_regular_sub_pos(root, shift, size)
                .visit(push_tail_visitor<node_t>{}, tail);
            return { shift, new_root };
        } else {
            return { shift, node_t::make_path(shift, tail) };
        }
    }

    rrbtree push_back(T value) const
    {
        auto ts = tail_size();
        if (ts < branches<B>) {
            auto new_tail = node_t::copy_leaf_emplace(tail, ts,
                                                      std::move(value));
            return { size + 1, shift, root->inc(), new_tail };
        } else {
            using std::get;
            auto new_tail = node_t::make_leaf_n(1u, std::move(value));
            auto tail_off = tail_offset();
            auto new_root = push_tail(root, shift, tail_off,
                                      tail->inc(), size - tail_off);
            return { size + 1, get<0>(new_root), get<1>(new_root), new_tail };
        }
    }

    std::tuple<const T*, std::size_t, std::size_t>
    array_for(std::size_t idx) const
    {
        using std::get;
        auto tail_off = tail_offset();
        if (idx >= tail_off) {
            return { tail->leaf() + (idx - tail_off), tail_off, size };
        } else {
            auto subs = visit_maybe_relaxed_sub(
                root, shift, tail_off,
                relaxed_array_for_visitor<T>(), idx);
            auto offset = get<1>(subs);
            auto first  = idx - offset;
            auto end    = first + get<2>(subs);
            return { get<0>(subs) + offset, first, end };
        }
    }

    const T& get(std::size_t index) const
    {
        return descend(get_visitor<T>(), index);
    }

    template <typename FnT>
    rrbtree update(std::size_t idx, FnT&& fn) const
    {
        auto tail_off  = tail_offset();
        if (idx >= tail_off) {
            auto tail_size = size - tail_off;
            auto new_tail  = make_leaf_sub_pos(tail, tail_size)
                .visit(update_visitor<node_t>{}, idx - tail_off, fn);
            return { size, shift, root->inc(), new_tail };
        } else {
            auto new_root  = visit_maybe_relaxed_sub(
                root, shift, tail_off,
                update_visitor<node_t>{}, idx, fn);
            return { size, shift, new_root, tail->inc() };
        }
    }

    rrbtree assoc(std::size_t idx, T value) const
    {
        return update(idx, [&] (auto&&) {
                return std::move(value);
            });
    }

    rrbtree take(std::size_t new_size) const
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
            auto r = visit_maybe_relaxed_sub(root, shift, tail_off, v, l);
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

    rrbtree drop(std::size_t elems) const
    {
        if (elems == 0) {
            return *this;
        } else if (elems >= size) {
            return empty;
        } else if (elems == tail_offset()) {
            return { size - elems, B, empty.root->inc(), tail->inc() };
        } else if (elems > tail_offset()) {
            auto tail_off = tail_offset();
            auto new_tail = node_t::copy_leaf(tail, elems - tail_off,
                                              size - tail_off);
            return { size - elems, B, empty.root->inc(), new_tail };
        } else {
            using std::get;
            auto v = slice_left_visitor<node_t>();
            auto r = visit_maybe_relaxed_sub(root, shift, tail_offset(), v, elems);
            auto new_root  = get<1>(r);
            auto new_shift = get<0>(r);
            return { size - elems, new_shift, new_root, tail->inc() };
        }
        return *this;
    }

    rrbtree concat(const rrbtree& r) const
    {
        using std::get;
        if (size == 0)
            return r;
        else if (r.size == 0)
            return *this;
        else if (r.tail_offset() == 0) {
            // just concat the tail, similar to push_back
            auto tail_offst = tail_offset();
            auto tail_size  = size - tail_offst;
            if (tail_size == branches<B>) {
                auto new_root = push_tail(root, shift, tail_offst,
                                          tail->inc(), tail_size);
                return { size + r.size, get<0>(new_root), get<1>(new_root),
                         r.tail->inc() };
            } else if (tail_size + r.size <= branches<B>) {
                auto new_tail = node_t::copy_leaf(tail, tail_size,
                                                  r.tail, r.size);
                return { size + r.size, shift, root->inc(), new_tail };
            } else {
                auto remaining = branches<B> - tail_size;
                auto add_tail  = node_t::copy_leaf(tail, tail_size,
                                                   r.tail, remaining);
                auto new_tail  = node_t::copy_leaf(r.tail, remaining, r.size);
                auto new_root  = push_tail(root, shift, tail_offst,
                                           add_tail, branches<B>);
                return { size + r.size, get<0>(new_root), get<1>(new_root),
                         new_tail };
            }
        } else {
            auto tail_offst = tail_offset();
            auto with_tail  = push_tail(root, shift, tail_offst,
                                        tail->inc(), size - tail_offst);
            auto lshift     = get<0>(with_tail);
            auto lroot      = get<1>(with_tail);
            assert(lroot->check(lshift, size));
            auto concated   = concat_sub_tree(
                size, lshift, lroot,
                r.tail_offset(), r.shift, r.root,
                true);
            auto new_shift  = get<0>(concated);
            auto new_root   = get<1>(concated);
            assert(new_shift == new_root->compute_shift());
            assert(new_root->check(new_shift, size + r.tail_offset()));
            dec_node(lroot, lshift, size);
            return { size + r.size, new_shift, new_root, r.tail->inc() };
        }
    }

    static std::tuple<unsigned, node_t*> concat_sub_tree(
        std::size_t lsize, unsigned lshift, node_t* lnode,
        std::size_t rsize, unsigned rshift, node_t* rnode,
        bool is_top)
    {
        IMMER_TRACE("concat_sub_tree: " << lshift << " <> " << rshift);
        using std::get;
        if (lshift > rshift) {
            auto lr = lnode->relaxed();
            auto lidx = lr
                ? lr->count - 1
                : ((lsize - 1) >> lshift) & mask<B>;
            auto llsize = lr
                ? lr->sizes[lidx] - (lidx ? lr->sizes[lidx - 1] : 0)
                : lsize - (lidx << lshift);
            auto cnode  = get<1>(
                concat_sub_tree(
                    llsize, lshift - B, lnode->inner() [lidx],
                    rsize, rshift, rnode,
                    false));
            auto result = concat_rebalance(
                lsize, lnode,
                llsize, cnode,
                0, nullptr,
                lshift, is_top);
            dec_node(cnode, lshift);
            return result;
        } else if (lshift < rshift) {
            auto rr = rnode->relaxed();
            auto rrsize = rr
                ? rr->sizes[0]
                : std::min(rsize, std::size_t{1} << rshift);
            auto cnode  = get<1>(
                concat_sub_tree(
                    lsize, lshift, lnode,
                    rrsize, rshift - B, rnode->inner() [0],
                    false));
            auto result = concat_rebalance(
                0, nullptr,
                rrsize, cnode,
                rsize, rnode,
                rshift, is_top);
            dec_node(cnode, rshift);
            return result;
        } else if (lshift == 0) {
            assert(lsize);
            assert(rsize);
            auto lslots = (((lsize - 1) >> lshift) & mask<B>) + 1;
            auto rslots = (((rsize - 1) >> lshift) & mask<B>) + 1;
            if (is_top && lslots + rslots <= branches<B>)
                return { 0, node_t::copy_leaf(lnode, lslots, rnode, rslots) };
            else {
                return { B, node_t::make_inner_r_n(2u,
                                                   lnode->inc(), lslots,
                                                   rnode->inc(), rslots) };
            }
        } else {
            auto lr = lnode->relaxed();
            auto lidx = lr
                ? lr->count - 1
                : ((lsize - 1) >> lshift) & mask<B>;
            auto llsize = lr
                ? lr->sizes[lidx] - (lidx ? lr->sizes[lidx - 1] : 0)
                : lsize - (lidx << lshift);
            auto rr = rnode->relaxed();
            auto rrsize = rr
                ? rr->sizes[0]
                : std::min(rsize, std::size_t{1} << rshift);
            auto cnode  = get<1>(
                concat_sub_tree(
                    llsize, lshift - B, lnode->inner() [lidx],
                    rrsize, rshift - B, rnode->inner() [0],
                    false));
            auto result = concat_rebalance(
                lsize, lnode,
                llsize + rrsize, cnode,
                rsize, rnode,
                lshift, is_top);
            dec_node(cnode, lshift);
            return result;
        }
    }

    struct leaf_policy
    {
        using data_t = T;
        static node_t* make(unsigned n) { return node_t::make_leaf_n(n); }
        static data_t* data(node_t* n) { return n->leaf(); }
        static typename node_t::relaxed_t* relaxed(node_t*) {
            return nullptr;
        }
        static std::size_t size(node_t*, unsigned slots) {
            return slots;
        }
        static void copied(node_t*, unsigned, std::size_t,
                           unsigned, unsigned,
                           node_t*, unsigned) {}
    };

    struct inner_r_policy
    {
        using data_t = node_t*;
        static node_t* make(unsigned n) { return node_t::make_inner_r_n(n); }
        static data_t* data(node_t* n) { return n->inner(); }
        static typename node_t::relaxed_t* relaxed(node_t* n) {
            return n->relaxed();
        }
        static std::size_t size(node_t* n, unsigned slots) {
            return n->relaxed()->sizes[slots - 1];
        }
        static void copied(node_t* from, unsigned shift, std::size_t size,
                           unsigned from_offset, unsigned copy_count,
                           node_t* to, unsigned to_offset)
        {
            auto tor = to->relaxed();
            tor->count += copy_count;
            node_t::inc_nodes(to->inner() + to_offset, copy_count);
            if (auto fromr = from->relaxed()) {
                auto last_from_size = from_offset ? fromr->sizes[from_offset - 1] : 0;
                auto last_to_size   = to_offset ? tor->sizes[to_offset - 1] : 0;
                for (auto i = 0; i < copy_count; ++i) {
                    last_to_size = tor->sizes [to_offset + i] =
                        fromr->sizes[from_offset + i] -
                        last_from_size +
                        last_to_size;
                    last_from_size = fromr->sizes[from_offset + i];
                }
            } else {
                auto lidx = ((size - 1) >> shift) & mask<B>;
                auto end = from_offset + copy_count;
                if (end < lidx) {
                    for (auto idx = from_offset; idx < end; ++idx)
                        tor->sizes[idx] = (idx + 1) << (shift - B);
                } else {
                    for (auto idx = from_offset; idx < lidx; ++idx)
                        tor->sizes[idx] = (idx + 1) << (shift - B);
                    tor->sizes[lidx] = size;
                }
            }
        }
    };

    template <typename ChildrenPolicy>
    struct node_merger
    {
        using policy = ChildrenPolicy;
        using data_t = typename policy::data_t;

        unsigned* slots;
        unsigned  slots_n;

        node_t*     result    = node_t::make_inner_r_n(std::min<unsigned>(
                                                           slots_n,
                                                           branches<B>));
        node_t*     parent    = result;
        node_t*     to        = nullptr;
        unsigned    to_offset = 0;
        std::size_t size_sum  = 0;

        node_merger(unsigned* s, unsigned n)
            : slots{s}, slots_n{n}
        {}

        std::tuple<unsigned, node_t*>
        finish(unsigned shift, bool is_top)
        {
            assert(!to);
            if (parent != result) {
                assert(result->relaxed()->count == 2);
                auto r = result->relaxed();
                r->sizes[1] = r->sizes[0] + size_sum;
                return { shift + B, result };
            } else if (is_top) {
                return { shift, result };
            } else {
                return { shift, node_t::make_inner_r_n(1u, result, size_sum) };
            }
        }

        void merge(node_t* node, unsigned shift, std::size_t size,
                   unsigned nslots, unsigned offset, unsigned endoff)
        {
            auto add_child = [&] (node_t* n, std::size_t size)
            {
                ++slots;
                auto r = parent->relaxed();
                if (r->count == branches<B>) {
                    assert(result == parent);
                    auto new_parent = node_t::make_inner_r_n(slots_n - branches<B>);
                    result = node_t::make_inner_r_n(2u, parent, size_sum, new_parent);
                    parent = new_parent;
                    r = new_parent->relaxed();
                    size_sum = 0;
                }
                auto idx = r->count++;
                r->sizes[idx] = size_sum += size;
                parent->inner() [idx] = n;
                assert(!idx || parent->inner() [idx] != parent->inner() [idx - 1]);
            };

            if (!node) return;
            auto r  = node->relaxed();
            auto nshift = shift - B;
            for (auto idx = offset; idx + endoff < nslots; ++idx) {
                auto from = node->inner() [idx];
                auto fromr = policy::relaxed(from);
                auto from_data  = policy::data(from);
                auto from_size  =
                    r                ? r->sizes[idx] - (idx ? r->sizes[idx - 1] : 0) :
                    idx < nslots - 1 ? 1 << shift
                    /* otherwise */  : size - (idx << shift);
                assert(from_size);
                auto from_slots = static_cast<unsigned>(
                    fromr           ? fromr->count
                    /* otherwise */ : ((from_size - 1) >> nshift) + 1);
                if (!to && *slots == from_slots) {
                    add_child(from->inc(), from_size);
                } else {
                    auto from_offset = 0u;
                    do {
                        if (!to) {
                            to = policy::make(*slots);
                            to_offset  = 0;
                        }
                        auto to_copy = std::min(
                            from_slots - from_offset,
                            *slots - to_offset);
                        auto data = policy::data(to);
                        assert(to_copy);
                        std::uninitialized_copy(
                            from_data + from_offset,
                            from_data + from_offset + to_copy,
                            data + to_offset);
                        policy::copied(
                            from, nshift, from_size, from_offset,
                            to_copy, to, to_offset);
                        to_offset   += to_copy;
                        from_offset += to_copy;
                        if (*slots == to_offset) {
                            add_child(to, policy::size(to, to_offset));
                            to = nullptr;
                        }
                    } while (from_slots != from_offset);
                }
            }
        }
    };

    struct concat_rebalance_plan_t
    {
        unsigned slots [3 * branches<B>];
        unsigned total_slots = 0u;
        unsigned count = 0u;
    };

    static std::tuple<unsigned, node_t*>
    concat_rebalance(std::size_t lsize, node_t* lnode,
                     std::size_t csize, node_t* cnode,
                     std::size_t rsize, node_t* rnode,
                     unsigned shift,
                     bool is_top)
    {
        assert(cnode);
        assert(lnode || rnode);

        // list all children and their slot counts
        unsigned all_slots [3 * branches<B>];
        auto total_all_slots = 0u;
        auto all_n = 0u;

        auto add_slots = [&] (node_t* node, unsigned shift, std::size_t size,
                              unsigned offset, unsigned endoff, unsigned& slots)
        {
            if (node) {
                auto nshift = shift - B;
                if (auto r = node->relaxed()) {
                    slots = r->count;
                    auto last_size = offset ? r->sizes[offset - 1] : 0;
                    for (auto i = offset; i + endoff < slots; ++i) {
                        auto nsize = r->sizes[i] - last_size;
                        total_all_slots += all_slots[all_n++] =
                            nshift == 0 || !node->inner()[i]->relaxed()
                            ? ((nsize - 1) >> nshift) + 1
                            : node->inner()[i]->relaxed()->count;
                        last_size = r->sizes[i];
                        assert(all_slots[all_n-1]);
                    }
                } else {
                    assert(size);
                    auto lidx = ((size - 1) >> shift) & mask<B>;
                    slots = lidx + 1;
                    auto i = offset;
                    for (; i + endoff < lidx; ++i)
                        total_all_slots += all_slots[all_n++] = branches<B>;
                    if (i + endoff < slots) {
                        total_all_slots += all_slots[all_n++] =
                            i == lidx
                            ? (((size - 1) >> nshift) & mask<B>) + 1
                            : branches<B>;
                        assert(all_slots[all_n-1]);
                    }
                }
            }
        };

        auto lslots = 0u;
        auto cslots = 0u;
        auto rslots = 0u;
        add_slots(lnode, shift, lsize, 0, 1, lslots);
        add_slots(cnode, shift, csize, 0, 0, cslots);
        add_slots(rnode, shift, rsize, 1, 0, rslots);

        // plan rebalance
        const auto RRB_EXTRAS = 2;
        const auto RRB_INVARIANT = 1;

        IMMER_TRACE("all_slots: " <<
                   pretty_print_array(all_slots, all_n));

        assert(total_all_slots > 0);
        const auto optimal_slots = ((total_all_slots - 1) / branches<B>) + 1;
        IMMER_TRACE("optimal_slots: " << optimal_slots <<
                   " // " << total_all_slots);

        auto shuffled_n = all_n;
        auto i = 0u;
        while (shuffled_n >= optimal_slots + RRB_EXTRAS) {
            // skip ok nodes
            while (all_slots[i] > branches<B> - RRB_INVARIANT) i++;
            // short node, redistribute
            auto remaining = all_slots[i];
            do {
                auto min_size = std::min(remaining + all_slots[i+1],
                                         branches<B, unsigned>);
                all_slots[i] = min_size;
                remaining += all_slots[i+1] - min_size;
                ++i;
            } while (remaining > 0);
            // remove node
            std::move(all_slots + i + 1, all_slots + shuffled_n, all_slots + i);
            --shuffled_n;
            --i;
        }

        IMMER_TRACE("    // " << pretty_print_array(all_slots, shuffled_n));

        // actually rebalance the nodes
        auto merge_all = [&] (auto& merger)
        {
            merger.merge(lnode, shift, lsize, lslots, 0, 1);
            merger.merge(cnode, shift, csize, cslots, 0, 0);
            merger.merge(rnode, shift, rsize, rslots, 1, 0);
            assert(!merger.to);
            assert(merger.slots == all_slots + shuffled_n);
            return merger.finish(shift, is_top);
        };

        if (shift == B) {
            auto merger = node_merger<leaf_policy>{all_slots, shuffled_n};
            return merge_all(merger);
        } else {
            auto merger = node_merger<inner_r_policy>{all_slots, shuffled_n};
            return merge_all(merger);
        }
    }

    bool check_tree() const
    {
#if IMMER_DEBUG_DEEP_CHECK
        assert(shift >= B);
        assert(tail_offset() <= size);
        assert(check_root());
        assert(check_tail());
#endif
        return true;
    }

    bool check_tail() const
    {
#if IMMER_DEBUG_DEEP_CHECK
        if (tail_size() > 0)
            assert(tail->check(0, tail_size()));
#endif
        return true;
    }

    bool check_root() const
    {
#if IMMER_DEBUG_DEEP_CHECK
        if (tail_offset() > 0)
            assert(root->check(shift, tail_offset()));
        else {
            assert(root->kind() == node_t::inner_kind);
            assert(shift == B);
        }
#endif
        return true;
    }

#if IMMER_DEBUG_PRINT
    void debug_print() const
    {
        std::cerr
            << "--" << std::endl
            << "{" << std::endl
            << "  size  = " << size << std::endl
            << "  shift = " << shift << std::endl
            << "  root  = " << std::endl;
        debug_print_node(root, shift, tail_offset());
        std::cerr << "  tail  = " << std::endl;
        debug_print_node(tail, 0, tail_size());
        std::cerr << "}" << std::endl;
    }

    void debug_print_indent(unsigned indent) const
    {
        while (indent --> 0)
            std::cerr << ' ';
    }

    void debug_print_node(node_t* node,
                          unsigned shift,
                          std::size_t size,
                          unsigned indent = 8) const
    {
        const auto indent_step = 4;

        if (shift == 0) {
            debug_print_indent(indent);
            std::cerr << "- {" << size << "} "
                      << pretty_print_array(node->leaf(), size)
                      << std::endl;
        } else if (auto r = node->relaxed()) {
            auto count = r->count;
            debug_print_indent(indent);
            std::cerr << "# {" << size << "} "
                      << pretty_print_array(r->sizes, r->count)
                      << std::endl;
            auto last_size = std::size_t{};
            for (auto i = 0; i < count; ++i) {
                debug_print_node(node->inner()[i],
                                 shift - B,
                                 r->sizes[i] - last_size,
                                 indent + indent_step);
                last_size = r->sizes[i];
            }
        } else {
            debug_print_indent(indent);
            std::cerr << "+ {" << size << "}" << std::endl;
            auto count = (size >> shift)
                + (size - ((size >> shift) << shift) > 0);
            if (count) {
                for (auto i = 0; i < count - 1; ++i)
                    debug_print_node(node->inner()[i],
                                     shift - B,
                                     1 << shift,
                                     indent + indent_step);
                debug_print_node(node->inner()[count - 1],
                                 shift - B,
                                 size - ((count - 1) << shift),
                                 indent + indent_step);
            }
        }
    }
#endif // IMMER_DEBUG_PRINT
};

template <typename T, int B, typename MP>
const rrbtree<T, B, MP> rrbtree<T, B, MP>::empty = {
    0,
    B,
    node_t::make_inner_n(0u),
    node_t::make_leaf_n(0u)
};

} // namespace rbts
} // namespace detail
} // namespace immer
