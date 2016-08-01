
#pragma once

#include <immu/detail/ivektor.hpp>

namespace immu {

template <typename T>
class ivektor
{
public:
    using value_type = T;
    using reference = const T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using const_reference = const T&;

    using iterator         = typename std::vector<T>::iterator;
    using const_iterator   = iterator;
    using reverse_iterator = typename std::vector<T>::reverse_iterator;

    ivektor() = default;

    iterator begin() const { return impl_.d->begin(); }
    iterator end()   const { return impl_.d->end(); }

    reverse_iterator rbegin() const { return impl_.d->rbegin(); }
    reverse_iterator rend()   const { return impl_.d->rend(); }

    std::size_t size() const { return impl_.d->size(); }
    bool empty() const { return impl_.d->empty(); }

    reference operator[] (size_type index) const
    { return impl_.get(index); }

    ivektor push_back(value_type value) const
    { return { impl_.push_back(std::move(value)) }; }

    ivektor assoc(std::size_t idx, value_type value) const
    { return { impl_.assoc(idx, std::move(value)) }; }

    template <typename FnT>
    ivektor update(std::size_t idx, FnT&& fn) const
    { return { impl_.update(idx, std::forward<FnT>(fn)) }; }

private:
    ivektor(detail::ivektor::impl<T> impl) : impl_(std::move(impl)) {}
    detail::ivektor::impl<T> impl_ = detail::ivektor::empty<T>;
};

} /* namespace immu */
