
#pragma once

#include <immu/detail/free_list.hpp>
#include <immu/detail/ref_count_base.hpp>

#include <boost/intrusive_ptr.hpp>
#include <boost/smart_ptr/intrusive_ref_counter.hpp>

#include <array>
#include <memory>
#include <cassert>

namespace immu {
namespace detail {

template <int B, typename T=std::size_t>
constexpr T branches = T{1} << B;

template <int B, typename T=std::size_t>
constexpr T mask = branches<B, T> - 1;

template <typename T, int B>
struct node;

template <typename T, int B>
using node_ptr = boost::intrusive_ptr<node<T, B> >;

template <typename T, int B>
using leaf_node  = std::array<T, 1 << B>;

template <typename T, int B>
using inner_node = std::array<node_ptr<T, B>, 1 << B>;

template <typename T, int B, typename Deriv=void>
struct node_base : ref_count_base<Deriv>
{
    using leaf_node_t  = leaf_node<T, B>;
    using inner_node_t = inner_node<T, B>;

    enum
    {
        leaf_kind,
        inner_kind
    } kind;

    union data_t
    {
        leaf_node_t  leaf;
        inner_node_t inner;
        data_t(leaf_node_t n)  : leaf(std::move(n)) {}
        data_t(inner_node_t n) : inner(std::move(n)) {}
        ~data_t() {}
    } data;

    ~node_base()
    {
        switch (kind) {
        case leaf_kind:
            data.leaf.~leaf_node_t();
            break;
        case inner_kind:
            data.inner.~inner_node_t();
            break;
        }
    }

    node_base(leaf_node<T, B> n)
        : kind{leaf_kind}
        , data{std::move(n)}
    {}

    node_base(inner_node<T, B> n)
        : kind{inner_kind}
        , data{std::move(n)}
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
struct node : node_base<T, B, node<T, B>>
            , with_thread_local_free_list<sizeof(node_base<T, B, node<T, B>>)>
{
    using node_base<T, B, node<T, B>>::node_base;
};

template <typename T, int B, typename ...Ts>
auto make_node(Ts&& ...xs) -> boost::intrusive_ptr<node<T, B>>
{
    return new node<T, B>(std::forward<Ts>(xs)...);
}

} /* namespace detail */
} /* namespace immu */
