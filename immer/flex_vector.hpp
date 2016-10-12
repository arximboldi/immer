
#pragma once

#include <immer/detail/rrbtree.hpp>
#include <immer/detail/rrbtree_iterator.hpp>
#include <immer/memory_policy.hpp>

namespace immer {

template <typename T, int B, typename MP>
class vector;

template <typename T,
          int B = default_bits,
          typename MemoryPolicy = default_memory_policy>
class flex_vector
{
    using impl_t = detail::rrbtree<T, B, MemoryPolicy>;

public:
    using value_type = T;
    using reference = const T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using const_reference = const T&;

    using iterator         = detail::rrbtree_iterator<T, B, MemoryPolicy>;
    using const_iterator   = iterator;
    using reverse_iterator = std::reverse_iterator<iterator>;

    flex_vector() = default;
    flex_vector(vector<T, B, MemoryPolicy> v)
        : impl_ { v.impl_.size, v.impl_.shift,
                  v.impl_.root->inc(), v.impl_.tail->inc() }
    {}

    iterator begin() const { return {impl_}; }
    iterator end()   const { return {impl_, typename iterator::end_t{}}; }

    reverse_iterator rbegin() const { return reverse_iterator{end()}; }
    reverse_iterator rend()   const { return reverse_iterator{begin()}; }

    std::size_t size() const { return impl_.size; }
    bool empty() const { return impl_.size == 0; }

    reference operator[] (size_type index) const
    { return impl_.get(index); }

    flex_vector push_back(value_type value) const
    { return { impl_.push_back(std::move(value)) }; }

    flex_vector assoc(std::size_t idx, value_type value) const
    { return { impl_.assoc(idx, std::move(value)) }; }

    template <typename FnT>
    flex_vector update(std::size_t idx, FnT&& fn) const
    { return { impl_.update(idx, std::forward<FnT>(fn)) }; }

    flex_vector take(std::size_t elems) const
    { return { impl_.take(elems) }; }

    flex_vector drop(std::size_t elems) const
    { return { impl_.drop(elems) }; }

    template <typename Step, typename State>
    State reduce(Step&& step, State&& init) const
    { return impl_.reduce(std::forward<Step>(step),
                          std::forward<State>(init)); }

    friend flex_vector operator+ (const flex_vector& l, const flex_vector& r)
    { return l.impl_.concat(r.impl_); }

    flex_vector push_front(value_type value) const
    { return flex_vector{}.push_back(value) + *this; }

#if IMMER_DEBUG_PRINT
    void debug_print() const
    { impl_.debug_print(); }
#endif

private:
    flex_vector(impl_t impl)
        : impl_(std::move(impl))
    {
#if IMMER_DEBUG_PRINT
        // force the compiler to generate debug_print, so we can call
        // it from a debugger
        [](volatile auto){}(&flex_vector::debug_print);
#endif
    }
    impl_t impl_ = impl_t::empty;
};

} // namespace immer
