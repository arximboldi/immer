
#pragma once

#include <immu/detail/dvektor.hpp>

#include <eggs/variant.hpp>

namespace immu {

template <typename T>
class dvektor
{
public:
    using value_type = T;
    using reference = const T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using const_reference = const T&;

    using iterator         = detail::dvektor::iterator<T>;
    using const_iterator   = iterator;
    using reverse_iterator = std::reverse_iterator<iterator>;

    dvektor() = default;

    iterator begin() const { return {impl_}; }
    iterator end()   const { return {impl_, typename iterator::end_t{}}; }

    reverse_iterator rbegin() const { return reverse_iterator{end()}; }
    reverse_iterator rend()   const { return reverse_iterator{begin()}; }

    std::size_t size() const { return impl_.size; }
    bool empty() const { return impl_.size == 0; }

    reference operator[] (size_type index) const
    { return impl_.get(index); }

    dvektor push_back(value_type value) const
    { return { impl_.push_back(std::move(value)) }; }

    dvektor assoc(std::size_t idx, value_type value) const
    { return { impl_.assoc(idx, std::move(value)) }; }

    template <typename FnT>
    dvektor update(std::size_t idx, FnT&& fn) const
    { return { impl_.update(idx, std::forward<FnT>(fn)) }; }

private:
    dvektor(detail::dvektor::impl<T> impl) : impl_(std::move(impl)) {}
    detail::dvektor::impl<T> impl_ = detail::dvektor::empty<T>;
};

} // namespace immu
