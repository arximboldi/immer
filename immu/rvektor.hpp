
#pragma once

#include <immu/detail/rrbtree.hpp>
#include <immu/detail/rrbtree_iterator.hpp>
#include <immu/memory_policy.hpp>

namespace immu {

template <typename T,
          int B = 5,
          typename MemoryPolicy = default_memory_policy>
class rvektor
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

    rvektor() = default;

    iterator begin() const { return {impl_}; }
    iterator end()   const { return {impl_, typename iterator::end_t{}}; }

    reverse_iterator rbegin() const { return reverse_iterator{end()}; }
    reverse_iterator rend()   const { return reverse_iterator{begin()}; }

    std::size_t size() const { return impl_.size; }
    bool empty() const { return impl_.size == 0; }

    reference operator[] (size_type index) const
    { return impl_.get(index); }

    rvektor push_back(value_type value) const
    { return { impl_.push_back(std::move(value)) }; }

    rvektor assoc(std::size_t idx, value_type value) const
    { return { impl_.assoc(idx, std::move(value)) }; }

    template <typename FnT>
    rvektor update(std::size_t idx, FnT&& fn) const
    { return { impl_.update(idx, std::forward<FnT>(fn)) }; }

    rvektor take(std::size_t elems) const
    { return { impl_.take(elems) }; }

    rvektor drop(std::size_t elems) const
    { return { impl_.drop(elems) }; }

    template <typename Step, typename State>
    State reduce(Step&& step, State&& init) const
    { return impl_.reduce(std::forward<Step>(step),
                          std::forward<State>(init)); }

    friend rvektor operator+ (const rvektor& l, const rvektor& r)
    { return l.impl_.concat(r.impl_); }

    rvektor push_front(value_type value) const
    { return rvektor{}.push_back(value) + *this; }

#if IMMU_DEBUG_PRINT
    void debug_print() const
    { impl_.debug_print(); }
#endif

private:
    rvektor(impl_t impl)
        : impl_(std::move(impl))
    {
#if IMMU_DEBUG_PRINT
        // force the compiler to generate debug_print, so we can call
        // it from a debugger
        [](volatile auto){}(&rvektor::debug_print);
#endif
    }
    impl_t impl_ = impl_t::empty;
};

} // namespace immu
