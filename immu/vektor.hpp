
#pragma once

#include <immu/detail/vektor.hpp>

namespace immu {

template <typename T, int B=5>
class vektor
{
public:
    using value_type = T;
    using reference = const T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using const_reference = const T&;

    using iterator         = detail::vektor::iterator<T, B>;
    using const_iterator   = iterator;
    using reverse_iterator = std::reverse_iterator<iterator>;

    vektor() = default;

    iterator begin() const { return {impl_}; }
    iterator end()   const { return {impl_, typename iterator::end_t{}}; }

    reverse_iterator rbegin() const { return reverse_iterator{end()}; }
    reverse_iterator rend()   const { return reverse_iterator{begin()}; }

    std::size_t size() const { return impl_.size; }
    bool empty() const { return impl_.size == 0; }

    reference operator[] (size_type index) const
    { return impl_.get(index); }

    vektor push_back(value_type value) const
    { return { impl_.push_back(std::move(value)) }; }

    vektor assoc(std::size_t idx, value_type value) const
    { return { impl_.assoc(idx, std::move(value)) }; }

    template <typename FnT>
    vektor update(std::size_t idx, FnT&& fn) const
    { return { impl_.update(idx, std::forward<FnT>(fn)) }; }

    template <typename Step, typename State>
    State reduce(Step&& step, State&& init) const
    { return impl_.reduce(std::forward<Step>(step),
                          std::forward<State>(init)); }

private:
    vektor(detail::vektor::impl<T, B> impl) : impl_(std::move(impl)) {}
    detail::vektor::impl<T, B> impl_ = detail::vektor::empty<T, B>;
};

} // namespace immu
