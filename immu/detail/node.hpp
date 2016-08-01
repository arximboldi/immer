
#pragma once

#include <immu/detail/free_list.hpp>

#include <boost/intrusive_ptr.hpp>
#include <boost/smart_ptr/intrusive_ref_counter.hpp>

#include <array>
#include <memory>
#include <atomic>
#include <cassert>

namespace immu {
namespace detail {

constexpr auto branching_log  = 5u;
constexpr auto branching      = 1u << branching_log;
constexpr std::size_t branching_mask = branching - 1;

template <typename T>
struct node;

template <typename T>
using node_ptr = boost::intrusive_ptr<node<T> >;

template <typename T>
using leaf_node  = std::array<T, branching>;

template <typename T>
using inner_node = std::array<node_ptr<T>, branching>;

template <typename T, typename Deriv=void>
struct node_base
{
    mutable std::atomic<int> ref_count;

    friend void intrusive_ptr_add_ref(const Deriv* x)
    {
        x->ref_count.fetch_add(1, std::memory_order_relaxed);
    }

    friend void intrusive_ptr_release(const Deriv* x)
    {
        if (x->ref_count.fetch_sub(1, std::memory_order_release) == 1) {
            std::atomic_thread_fence(std::memory_order_acquire);
            delete x;
        }
    }

    enum
    {
        leaf_kind,
        inner_kind
    } kind;

    union data_t
    {
        leaf_node<T>  leaf;
        inner_node<T> inner;
        data_t(leaf_node<T> n)  : leaf(std::move(n)) {}
        data_t(inner_node<T> n) : inner(std::move(n)) {}
        ~data_t() {}
    } data;

    ~node_base()
    {
        switch (kind) {
        case leaf_kind:
            data.leaf.~leaf_node<T>();
            break;
        case inner_kind:
            data.inner.~inner_node<T>();
            break;
        }
    }

    node_base(leaf_node<T> n)
        : kind{leaf_kind}
        , data{std::move(n)}
    {}

    node_base(inner_node<T> n)
        : kind{inner_kind}
        , data{std::move(n)}
    {}

    inner_node<T>& inner() & {
        assert(kind == inner_kind);
        return data.inner;
    }
    const inner_node<T>& inner() const& {
        assert(kind == inner_kind);
        return data.inner;
    }
    inner_node<T>&& inner() && {
        assert(kind == inner_kind);
        return std::move(data.inner);
    }

    leaf_node<T>& leaf() & {
        assert(kind == leaf_kind);
        return data.leaf;
    }
    const leaf_node<T>& leaf() const& {
        assert(kind == leaf_kind);
        return data.leaf;
    }
    leaf_node<T>&& leaf() && {
        assert(kind == leaf_kind);
        return std::move(data.leaf);
    }
};

template <typename T>
struct node : node_base<T, node<T>>
            , with_thread_local_free_list<sizeof(node_base<T, node<T>>)>
{
    using node_base<T, node<T>>::node_base;
};

template <typename T, typename ...Ts>
auto make_node(Ts&& ...xs) -> boost::intrusive_ptr<node<T>>
{
    return new node<T>(std::forward<Ts>(xs)...);
}

} /* namespace detail */
} /* namespace immu */
