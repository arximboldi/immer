
#pragma once

#include <immer/detail/rbtree.hpp>
#include <immer/detail/rbtree_iterator.hpp>
#include <immer/memory_policy.hpp>

namespace immer {

template <typename T,
          int B = 5,
          typename MemoryPolicy = default_memory_policy>
class vector
{
    using impl_t = detail::rbtree<T, B, MemoryPolicy>;

public:
    using value_type = T;
    using reference = const T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using const_reference = const T&;

    using iterator         = detail::rbtree_iterator<T, B, MemoryPolicy>;
    using const_iterator   = iterator;
    using reverse_iterator = std::reverse_iterator<iterator>;

    vector() = default;

    iterator begin() const { return {impl_}; }
    iterator end()   const { return {impl_, typename iterator::end_t{}}; }

    reverse_iterator rbegin() const { return reverse_iterator{end()}; }
    reverse_iterator rend()   const { return reverse_iterator{begin()}; }

    std::size_t size() const { return impl_.size; }
    bool empty() const { return impl_.size == 0; }

    reference operator[] (size_type index) const
    { return impl_.get(index); }

    vector push_back(value_type value) const
    { return { impl_.push_back(std::move(value)) }; }

    vector assoc(std::size_t idx, value_type value) const
    { return { impl_.assoc(idx, std::move(value)) }; }

    template <typename FnT>
    vector update(std::size_t idx, FnT&& fn) const
    { return { impl_.update(idx, std::forward<FnT>(fn)) }; }

    vector take(std::size_t elems) const
    { return { impl_.take(elems) }; }

    template <typename Step, typename State>
    State reduce(Step&& step, State&& init) const
    { return impl_.reduce(std::forward<Step>(step),
                          std::forward<State>(init)); }

private:
    vector(impl_t impl) : impl_(std::move(impl)) {}
    impl_t impl_ = impl_t::empty;
};

} // namespace immer
