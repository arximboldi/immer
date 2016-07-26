
#pragma once

#include <eggs/variant.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <memory>

namespace immu {
namespace detail {
namespace vektor {

constexpr auto branching_log  = 5;
constexpr auto branching      = 1 << branching_log;
constexpr auto branching_mask = branching - 1;

template <typename T>
struct node;

template <typename T>
using node_ptr = std::shared_ptr<node<T> >;

template <typename T>
using leaf_node  = std::array<T, branching>;

template <typename T>
using inner_node = std::array<node_ptr<T>, branching>;

using eggs::variants::get;

template <typename T>
struct node : eggs::variant<leaf_node<T>, inner_node<T>>
{
    using base_t = eggs::variant<leaf_node<T>, inner_node<T>>;
    using base_t::base_t;

    inner_node<T>& inner() & { return get<inner_node<T>>(*this); }
    const inner_node<T>& inner() const& { return get<inner_node<T>>(*this); }
    inner_node<T>&& inner() && { return get<inner_node<T>>(std::move(*this)); }

    leaf_node<T>& leaf() & { return get<leaf_node<T>>(*this); }
    const leaf_node<T>& leaf() const& { return get<leaf_node<T>>(*this); }
    leaf_node<T>&& leaf() && { return get<leaf_node<T>>(std::move(*this)); }
};

template <typename T, typename ...Ts>
auto make_node(Ts&& ...xs)
{
    return std::make_shared<node<T>>(std::forward<Ts>(xs)...);
}

template <typename T>
struct impl
{
    using inner_t    = inner_node<T>;
    using leaf_t     = leaf_node<T>;
    using node_t     = node<T>;
    using node_ptr_t = node_ptr<T>;

    std::size_t size;
    unsigned    shift;
    node_ptr_t  root;
    node_ptr_t  tail;

    auto tail_offset() const
    {
        return size < 32 ? 0 : ((size - 1) >> branching_log) << branching_log;
    }

    template <typename ...Ts>
    static auto make_node(Ts&& ...xs)
    {
        return vektor::make_node<T>(std::forward<Ts>(xs)...);
    }

    const leaf_t& array_for(std::size_t index) const
    {
        assert(index < size);

        if (index >= tail_offset())
            return tail->leaf();
        else {
            auto node = root.get();
            for (auto level = shift; level; level -= branching_log) {
                node = node->inner() [(index >> level) & branching_mask].get();
            }
            return node->leaf();
        }
    }

    node_ptr_t make_path(unsigned level, node_ptr_t node) const
    {
        return level == 0
            ? node
            : make_node(inner_t{{ make_path(level - branching_log,
                                            std::move(node)) }});
    }

    node_ptr_t push_tail(unsigned level,
                         const node_t& parent_,
                         node_ptr_t tail) const
    {
        const auto& parent   = parent_.inner();
        auto new_parent_node = make_node(parent);
        auto& new_parent     = new_parent_node->inner();
        auto idx             = ((size - 1) >> level) & branching_mask;
        auto next_node =
            level == branching_log ? std::move(tail) :
            parent[idx]            ? push_tail(level - branching_log,
                                               *parent[idx],
                                               std::move(tail)) :
            /* otherwise */          make_path(level - branching_log,
                                               std::move(tail));
        new_parent[idx] = next_node;
        return new_parent_node;
    }

    impl push_back(T value) const
    {
        auto tail_size = size - tail_offset();
        if (tail_size < branching) {
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
            return ((size >> branching_log) > (1u << shift))
                ? impl{ size + 1,
                    shift + branching_log,
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
        return arr [index & branching_mask];
    }

    template <typename FnT>
    impl update(std::size_t idx, FnT&& fn) const
    {
        if (idx >= tail_offset()) {
            const auto& old_tail = tail->leaf();
            auto new_tail_node  = make_node(leaf_t{old_tail});
            auto& item = new_tail_node->leaf() [idx & branching_mask];
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
            auto& item     = new_node->leaf() [idx & branching_mask];
            auto new_value = std::forward<FnT>(fn) (std::move(item));
            item = std::move(new_value);
            return new_node;
        } else {
            auto new_node = make_node(inner_t{node.inner()});
            auto& item    = new_node->inner()[(idx >> level) & branching_mask];
            item = do_update(level - branching_log, *item, idx, std::forward<FnT>(fn));
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

template <typename T> const auto    empty_inner = make_node<T>(inner_node<T>{});
template <typename T> const auto    empty_leaf  = make_node<T>(leaf_node<T>{});
template <typename T> const impl<T> empty       = {
    0,
    branching_log,
    empty_inner<T>,
    empty_leaf<T>
};

template <typename T>
struct iterator : boost::iterator_facade<
    iterator<T>,
    T,
    boost::random_access_traversal_tag,
    const T&>
{
    struct end_t {};

    iterator() = default;

    iterator(const impl<T>& v)
        : v_    { &v }
        , i_    { 0 }
        , base_ { 0 }
        , curr_ { v.array_for(0).begin() }
    {
    }

    iterator(const impl<T>& v, end_t)
        : v_    { &v }
        , i_    { v.size }
        , base_ { i_ - (i_ & branching_mask) }
        , curr_ { v.array_for(i_ - 1).begin() + (i_ - base_) }
    {}

private:
    friend class boost::iterator_core_access;

    using leaf_iterator = typename leaf_node<T>::const_iterator;

    const impl<T>* v_;
    std::size_t    i_;
    std::size_t    base_;
    leaf_iterator  curr_;

    void increment()
    {
        assert(i_ < v_->size);
        ++i_;
        if (i_ - base_ < branching) {
            ++curr_;
        } else {
            base_ += branching;
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
            base_ -= branching;
            curr_ = std::prev(v_->array_for(i_).end());
        }
    }

    void advance(std::ptrdiff_t n)
    {
        assert(n <= 0 || i_ + static_cast<std::size_t>(n) <= v_->size);
        assert(n >= 0 || static_cast<std::size_t>(-n) <= i_);

        i_ += n;
        if (i_ <= base_ && i_ - base_ < branching) {
            curr_ += n;
        } else {
            base_ = i_ - (i_ & branching_mask);
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
