#pragma once

#include <immer/config.hpp>
#include <immer/detail/hamts/champ.hpp>
#include <immer/detail/hamts/champ_iterator.hpp>
#include <immer/memory_policy.hpp>
#include <type_traits>

namespace immer {

template <typename T,
          typename KeyExtractor,
          typename Hash,
          typename Equal,
          typename MemoryPolicy,
          detail::hamts::bits_t B>
class single_hash_index_transient;

template <class T>
struct default_key_extractor
{
    using key_type = decltype(T().id);

    key_type operator()(const T& t) const { return t.id; }
};

template <typename T,
          typename KeyExtractor = default_key_extractor<T>,
          typename Hash         = std::hash<typename KeyExtractor::key_type>,
          typename Equal = std::equal_to<typename KeyExtractor::key_type>,
          typename MemoryPolicy   = default_memory_policy,
          detail::hamts::bits_t B = default_bits>
class single_hash_index
{
    using K       = typename KeyExtractor::key_type;
    using value_t = T;

    using move_t =
        std::integral_constant<bool, MemoryPolicy::use_transient_rvalues>;

    struct project_value
    {
        const T& operator()(const value_t& v) const noexcept { return v; }
    };

    struct project_value_ptr
    {
        const T* operator()(const value_t& v) const noexcept
        {
            return std::addressof(v);
        }
    };

    struct combine_value
    {
        template <typename Kf, typename Tf>
        value_t operator()(Kf&& k, Tf&& v) const
        {
            return std::forward<Tf>(v);
        }
    };

    struct default_value
    {
        const T& operator()() const
        {
            static T v{};
            return v;
        }
    };

    struct error_value
    {
        const T& operator()() const
        {
            IMMER_THROW(std::out_of_range{"key not found"});
        }
    };

    struct hash_key
    {
        auto operator()(const value_t& v) { return Hash{}(KeyExtractor{}(v)); }

        template <typename Key>
        auto operator()(const Key& v)
        {
            return Hash{}(v);
        }
    };

    struct equal_key
    {
        auto operator()(const value_t& a, const value_t& b)
        {
            auto ke = KeyExtractor{};
            return Equal{}(ke(a), ke(b));
        }

        template <typename Key>
        auto operator()(const value_t& a, const Key& b)
        {
            return Equal{}(KeyExtractor{}(a), b);
        }
    };

    struct equal_value
    {
        auto operator()(const value_t& a, const value_t& b) { return a == b; }
    };

    using impl_t =
        detail::hamts::champ<value_t, hash_key, equal_key, MemoryPolicy, B>;

public:
    using key_type        = K;
    using mapped_type     = T;
    using value_type      = T;
    using size_type       = detail::hamts::size_t;
    using diference_type  = std::ptrdiff_t;
    using hasher          = Hash;
    using key_equal       = Equal;
    using reference       = const value_type&;
    using const_reference = const value_type&;

    using iterator = detail::hamts::
        champ_iterator<value_t, hash_key, equal_key, MemoryPolicy, B>;
    using const_iterator = iterator;

    using transient_type = single_hash_index_transient<T, KeyExtractor, Hash, Equal, MemoryPolicy, B>;

    single_hash_index(std::initializer_list<value_type> values)
    {
        for (auto&& v : values)
            *this = std::move(*this).insert(v);
    }

    template <typename Iter,
              typename Sent,
              std::enable_if_t<detail::compatible_sentinel_v<Iter, Sent>,
                               bool> = true>
    single_hash_index(Iter first, Sent last)
    {
        for (; first != last; ++first)
            *this = std::move(*this).insert(*first);
    }

    single_hash_index() = default;

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
        return impl_.template get<project_value, default_value>(k);
    }

    IMMER_NODISCARD const T& operator[](const K& k) const
    {
        return impl_.template get<project_value, default_value>(k);
    }

    template <typename Key,
              typename U = Hash,
              typename   = typename U::is_transparent>
    const T& at(const Key& k) const
    {
        return impl_.template get<project_value, error_value>(k);
    }
    const T& at(const K& k) const
    {
        return impl_.template get<project_value, error_value>(k);
    }

    IMMER_NODISCARD const T* find(const K& k) const
    {
        return impl_.template get<project_value_ptr,
                                  detail::constantly<const T*, nullptr>>(k);
    }
    template <typename Key,
              typename U = Hash,
              typename   = typename U::is_transparent>
    IMMER_NODISCARD const T* find(const Key& k) const
    {
        return impl_.template get<project_value_ptr,
                                  detail::constantly<const T*, nullptr>>(k);
    }

    IMMER_NODISCARD bool operator==(const single_hash_index& other) const
    {
        return impl_.template equals<equal_value>(other.impl_);
    }
    IMMER_NODISCARD bool operator!=(const single_hash_index& other) const
    {
        return !(*this == other);
    }

    IMMER_NODISCARD single_hash_index insert(value_type value) const&
    {
        return impl_.add(std::move(value));
    }
    IMMER_NODISCARD decltype(auto) insert(value_type value) &&
    {
        return insert_move(move_t{}, std::move(value));
    }

    template <typename Fn>
    IMMER_NODISCARD single_hash_index update(key_type k, Fn&& fn) const&
    {
        return impl_
            .template update<project_value, default_value, combine_value>(
                std::move(k), std::forward<Fn>(fn));
    }
    template <typename Fn>
    IMMER_NODISCARD decltype(auto) update(key_type k, Fn&& fn) &&
    {
        return update_move(move_t{}, std::move(k), std::forward<Fn>(fn));
    }

    IMMER_NODISCARD single_hash_index erase(const K& k) const&
    {
        return impl_.sub(k);
    }

    IMMER_NODISCARD decltype(auto) erase(const K& k) &&
    {
        return erase_move(move_t{}, k);
    }

    IMMER_NODISCARD transient_type transient() const&
    {
        return transient_type{impl_};
    }
    IMMER_NODISCARD transient_type transient() &&
    {
        return transient_type{std::move(impl_)};
    }

    const impl_t& impl() const { return impl_; }

private:
    friend transient_type;

    single_hash_index&& insert_move(std::true_type, value_type value)
    {
        // xxx: implement mutable version
        impl_ = impl_.add(std::move(value));
        return std::move(*this);
    }

    single_hash_index insert_move(std::false_type, value_type value)
    {
        return impl_.add(std::move(value));
    }

    template <typename Fn>
    single_hash_index&& update_move(std::true_type, key_type k, Fn&& fn)
    {
        // xxx: implement mutable version
        impl_ =
            impl_.template update<project_value, default_value, combine_value>(
                std::move(k), std::forward<Fn>(fn));
        return std::move(*this);
    }

    template <typename Fn>
    single_hash_index update_move(std::false_type, key_type k, Fn&& fn)
    {
        return impl_
            .template update<project_value, default_value, combine_value>(
                std::move(k), std::forward<Fn>(fn));
    }

    single_hash_index&& erase_move(std::true_type, const key_type& value)
    {
        // xxx: implement mutable version
        impl_ = impl_.sub(value);
        return std::move(*this);
    }

    single_hash_index erase_move(std::false_type, const key_type& value)
    {
        return impl_.sub(value);
    }

    single_hash_index(impl_t impl)
        : impl_(std::move(impl))
    {}

    impl_t impl_ = impl_t::empty();
};

} // namespace immer
