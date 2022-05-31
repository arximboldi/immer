#pragma once

#include <immer/detail/hamts/champ.hpp>
#include <immer/memory_policy.hpp>
#include <type_traits>

namespace immer {

template <typename T,
          typename KeyFn,
          typename Hash,
          typename Equal,
          typename MemoryPolicy,
          detail::hamts::bits_t B>
class table;

template <typename T,
          typename KeyFn,
          typename Hash,
          typename Equal,
          typename MemoryPolicy,
          detail::hamts::bits_t B>
class table_transient : MemoryPolicy::transience_t::owner
{
    using K = std::decay_t<decltype(KeyFn{}(std::declval<T>()))>;
    using base_t  = typename MemoryPolicy::transience_t::owner;
    using owner_t = base_t;

public:
    using persistent_type = table<T, KeyFn, Hash, Equal, MemoryPolicy, B>;
    using key_type        = K;
    using mapped_type     = T;
    using value_type      = T;
    using size_type       = detail::hamts::size_t;
    using diference_type  = std::ptrdiff_t;
    using hasher          = Hash;
    using key_equal       = Equal;
    using reference       = const value_type&;
    using const_reference = const value_type&;

    using iterator       = typename persistent_type::iterator;
    using const_iterator = iterator;

    table_transient() = default;

    IMMER_NODISCARD iterator begin() const { return {impl_}; }

    IMMER_NODISCARD iterator end() const
    {
        return {impl_, typename iterator::end_t{}};
    }

    IMMER_NODISCARD size_type size() const { return impl_.size; }

    IMMER_NODISCARD bool empty() const { return impl_.size == 0; }

    template <typename Key,
              typename U = Hash,
              typename   = typename U::is_transparent>
    IMMER_NODISCARD size_type count(const Key& k) const
    {
        return impl_.template get<detail::constantly<size_type, 1>,
                                  detail::constantly<size_type, 0>>(k);
    }

    IMMER_NODISCARD size_type count(const K& k) const
    {
        return impl_.template get<detail::constantly<size_type, 1>,
                                  detail::constantly<size_type, 0>>(k);
    }

    template <typename Key,
              typename U = Hash,
              typename   = typename U::is_transparent>
    IMMER_NODISCARD const T& operator[](const Key& k) const
    {
        return impl_.template get<typename persistent_type::project_value,
                                  typename persistent_type::default_value>(k);
    }

    IMMER_NODISCARD const T& operator[](const K& k) const
    {
        return impl_.template get<typename persistent_type::project_value,
                                  typename persistent_type::default_value>(k);
    }

    template <typename Key,
              typename U = Hash,
              typename   = typename U::is_transparent>
    const T& at(const Key& k) const
    {
        return impl_.template get<typename persistent_type::project_value,
                                  typename persistent_type::error_value>(k);
    }

    const T& at(const K& k) const
    {
        return impl_.template get<typename persistent_type::project_value,
                                  typename persistent_type::error_value>(k);
    }

    IMMER_NODISCARD const T* find(const K& k) const
    {
        return impl_.template get<typename persistent_type::project_value_ptr,
                                  detail::constantly<const T*, nullptr>>(k);
    }

    template <typename Key,
              typename U = Hash,
              typename   = typename U::is_transparent>
    IMMER_NODISCARD const T* find(const Key& k) const
    {
        return impl_.template get<typename persistent_type::project_value_ptr,
                                  detail::constantly<const T*, nullptr>>(k);
    }

    void insert(value_type value)
    {
        // xxx: implement mutable version
        impl_ = impl_.add(std::move(value));
    }

    void set(key_type k, mapped_type v)
    {
        // xxx: implement mutable version
        impl_ = impl_.add({std::move(k), std::move(v)});
    }

    template <typename Fn>
    void update(key_type k, Fn&& fn)
    {
        // xxx: implement mutable version
        impl_ = impl_.template update<persistent_type::project_value,
                                      persistent_type::default_value,
                                      persistent_type::combine_value>(
            std::move(k), std::forward<Fn>(fn));
    }

    void erase(const K& k)
    {
        // xxx: implement mutable version
        impl_ = impl_.sub(k);
    }

    IMMER_NODISCARD persistent_type persistent() &
    {
        this->owner_t::operator=(owner_t{});
        return impl_;
    }
    IMMER_NODISCARD persistent_type persistent() && { return std::move(impl_); }

private:
    friend persistent_type;
    using impl_t = typename persistent_type::impl_t;

    table_transient(impl_t impl)
        : impl_(std::move(impl))
    {}

    impl_t impl_ = impl_t::empty();

public:
    // Semi-private
    const impl_t& impl() const { return impl_; }
};

} // namespace immer
