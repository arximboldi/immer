#pragma once

#include <cereal/archives/json.hpp>

#include <boost/hana.hpp>

/**
 * Special types of archives, working with JSON, that support providing extra
 * context (Pools) to serialize immer data structures.
 */

namespace immer::persist {

namespace detail {

/**
 * @brief This struct has an interface of an output archive but does
 * nothing when requested to serialize data. It is used when we only care about
 * the pools and have no need to serialize the actual document.
 *
 * @ingroup persist-api
 */
struct blackhole_output_archive
{
    void setNextName(const char* name) {}

    template <class T>
    friend void CEREAL_SAVE_FUNCTION_NAME(blackhole_output_archive& ar,
                                          cereal::NameValuePair<T> const& t)
    {
    }

    friend void CEREAL_SAVE_FUNCTION_NAME(blackhole_output_archive& ar,
                                          std::nullptr_t const& t)
    {
    }

    template <class T,
              cereal::traits::EnableIf<std::is_arithmetic<T>::value> =
                  cereal::traits::sfinae>
    friend void CEREAL_SAVE_FUNCTION_NAME(blackhole_output_archive& ar,
                                          T const& t)
    {
    }

    template <class CharT, class Traits, class Alloc>
    friend void CEREAL_SAVE_FUNCTION_NAME(
        blackhole_output_archive& ar,
        std::basic_string<CharT, Traits, Alloc> const& str)
    {
    }

    template <class T>
    friend void CEREAL_SAVE_FUNCTION_NAME(blackhole_output_archive& ar,
                                          cereal::SizeTag<T> const& v)
    {
    }
};

// blackhole_output_archive doesn't care about names
struct empty_name_fn
{
    void operator()(const auto& container) const {}
};

template <class Pools>
constexpr bool is_pool_empty()
{
    using Result =
        decltype(boost::hana::is_empty(boost::hana::keys(Pools{}.storage())));
    return Result::value;
}

} // namespace detail

/**
 * Adapted from cereal/archives/adapters.hpp
 */

/**
 * @brief An output archive wrapper that provides access to the ``Pools`` stored
 * inside. And serializes the ``pools`` object alongside the user document.
 *
 * @tparam Previous Type of the ``cereal`` output archive into which all the
 * serialization calls are forwarded.
 * @tparam Pools The wrapped pools. See ``output_pools``.
 * @tparam WrapFn A function that is called on each serialized value in order to
 * wrap the desired ``immer`` containers into ``persistable``.
 * @tparam PoolNameFn A function that is used to determine the name for each
 * individual pool. It's called with an empty ``immer`` container and must
 * return the name for the corresponding pool.
 *
 * @ingroup persist-api
 */
template <class Previous, class Pools, class WrapFn, class PoolNameFn>
class json_immer_output_archive
    : public cereal::OutputArchive<
          json_immer_output_archive<Previous, Pools, WrapFn, PoolNameFn>>
    , public cereal::traits::TextArchive
{
public:
    using pool_name_fn = PoolNameFn;

    template <class... Args>
    explicit json_immer_output_archive(Args&&... args)
        requires std::is_same_v<WrapFn, boost::hana::id_t>
        : cereal::OutputArchive<json_immer_output_archive>{this}
        , previous{std::forward<Args>(args)...}
    {
    }

    template <class... Args>
    json_immer_output_archive(Pools pools_, Args&&... args)
        requires std::is_same_v<WrapFn, boost::hana::id_t>
        : cereal::OutputArchive<json_immer_output_archive>{this}
        , previous{std::forward<Args>(args)...}
        , pools{std::move(pools_)}
    {
    }

    template <class... Args>
    json_immer_output_archive(Pools pools_, WrapFn wrap_, Args&&... args)
        : cereal::OutputArchive<json_immer_output_archive>{this}
        , wrap{std::move(wrap_)}
        , previous{std::forward<Args>(args)...}
        , pools{std::move(pools_)}
    {
    }

    ~json_immer_output_archive() { finalize(); }

    Pools& get_output_pools() & { return pools; }
    Pools&& get_output_pools() && { return std::move(pools); }

    void finalize()
    {
        if constexpr (detail::is_pool_empty<Pools>()) {
            return;
        }

        if (finalized) {
            return;
        }
        finalized = true;

        save_pools_impl();

        auto& self = *this;
        self(CEREAL_NVP(pools));
    }

    template <class T>
    friend void prologue(json_immer_output_archive& ar, T&& v)
    {
        using cereal::prologue;
        prologue(ar.previous, std::forward<T>(v));
    }

    template <class T>
    friend void epilogue(json_immer_output_archive& ar, T&& v)
    {
        using cereal::epilogue;
        epilogue(ar.previous, std::forward<T>(v));
    }

    template <class T>
    friend void CEREAL_SAVE_FUNCTION_NAME(json_immer_output_archive& ar,
                                          cereal::NameValuePair<T> const& t)
    {
        ar.previous.setNextName(t.name);
        ar(ar.wrap(t.value));
    }

    friend void CEREAL_SAVE_FUNCTION_NAME(json_immer_output_archive& ar,
                                          std::nullptr_t const& t)
    {
        using cereal::CEREAL_SAVE_FUNCTION_NAME;
        CEREAL_SAVE_FUNCTION_NAME(ar.previous, t);
    }

    template <class T,
              cereal::traits::EnableIf<std::is_arithmetic<T>::value> =
                  cereal::traits::sfinae>
    friend void CEREAL_SAVE_FUNCTION_NAME(json_immer_output_archive& ar,
                                          T const& t)
    {
        using cereal::CEREAL_SAVE_FUNCTION_NAME;
        CEREAL_SAVE_FUNCTION_NAME(ar.previous, t);
    }

    template <class CharT, class Traits, class Alloc>
    friend void CEREAL_SAVE_FUNCTION_NAME(
        json_immer_output_archive& ar,
        std::basic_string<CharT, Traits, Alloc> const& str)
    {
        using cereal::CEREAL_SAVE_FUNCTION_NAME;
        CEREAL_SAVE_FUNCTION_NAME(ar.previous, str);
    }

    template <class T>
    friend void CEREAL_SAVE_FUNCTION_NAME(json_immer_output_archive& ar,
                                          cereal::SizeTag<T> const& v)
    {
        using cereal::CEREAL_SAVE_FUNCTION_NAME;
        CEREAL_SAVE_FUNCTION_NAME(ar.previous, v);
    }

private:
    template <class Previous_, class Pools_, class WrapFn_, class PoolNameFn_>
    friend class json_immer_output_archive;

    // Recursively serializes the pools but not calling finalize
    void save_pools_impl()
    {
        const auto save_pool = [wrap = wrap](auto pools) {
            auto ar =
                json_immer_output_archive<detail::blackhole_output_archive,
                                          Pools,
                                          decltype(wrap),
                                          detail::empty_name_fn>{pools, wrap};
            // Do not try to serialize pools again inside of this temporary
            // archive
            ar.finalized = true;
            ar(pools);
            return std::move(ar).get_output_pools();
        };

        auto prev = pools;
        while (true) {
            // Keep saving pools until everything is saved.
            pools = save_pool(std::move(pools));
            if (prev == pools) {
                break;
            }
            prev = pools;
        }
    }

private:
    WrapFn wrap;
    Previous previous;
    Pools pools;
    bool finalized{false};
};

template <class T, class = void>
struct has_has_name_t : std::false_type
{};

template <class T>
struct has_has_name_t<T, std::void_t<decltype(std::declval<T>().hasName(""))>>
    : std::true_type
{};

template <class Previous, class Pools, class WrapFn, class PoolNameFn>
class json_immer_input_archive
    : public cereal::InputArchive<
          json_immer_input_archive<Previous, Pools, WrapFn, PoolNameFn>>
    , public cereal::traits::TextArchive
{
public:
    using pool_name_fn = PoolNameFn;

    template <class... Args>
    json_immer_input_archive(Pools pools_, Args&&... args)
        requires std::is_same_v<WrapFn, boost::hana::id_t>
        : cereal::InputArchive<json_immer_input_archive>{this}
        , previous{std::forward<Args>(args)...}
        , pools{std::move(pools_)}
    {
    }

    template <class... Args>
    json_immer_input_archive(Pools pools_, WrapFn wrap_, Args&&... args)
        : cereal::InputArchive<json_immer_input_archive>{this}
        , wrap{std::move(wrap_)}
        , previous{std::forward<Args>(args)...}
        , pools{std::move(pools_)}
    {
    }

    template <class Container>
    auto& get_loader()
    {
        auto& loader = loaders[boost::hana::type_c<Container>];
        if (!loader) {
            const auto& type_pool = pools.template get_pool<Container>();
            loader.emplace(type_pool.pool, type_pool.transform);
        }
        return *loader;
    }

    /**
     * This is needed for optional_nvp in the custom version of cereal.
     */
    std::enable_if_t<has_has_name_t<Previous>::value, bool>
    hasName(const char* name)
    {
        return previous.hasName(name);
    }

    template <class T>
    friend void prologue(json_immer_input_archive& ar, T&& v)
    {
        using cereal::prologue;
        prologue(ar.previous, std::forward<T>(v));
    }

    template <class T>
    friend void epilogue(json_immer_input_archive& ar, T&& v)
    {
        using cereal::epilogue;
        epilogue(ar.previous, std::forward<T>(v));
    }

    template <class T>
    friend void CEREAL_LOAD_FUNCTION_NAME(json_immer_input_archive& ar,
                                          cereal::NameValuePair<T>& t)
    {
        ar.previous.setNextName(t.name);
        auto&& wrapped = ar.wrap(t.value);
        ar(wrapped);
    }

    friend void CEREAL_LOAD_FUNCTION_NAME(json_immer_input_archive& ar,
                                          std::nullptr_t& t)
    {
        using cereal::CEREAL_LOAD_FUNCTION_NAME;
        CEREAL_LOAD_FUNCTION_NAME(ar.previous, t);
    }

    template <class T,
              cereal::traits::EnableIf<std::is_arithmetic<T>::value> =
                  cereal::traits::sfinae>
    friend void CEREAL_LOAD_FUNCTION_NAME(json_immer_input_archive& ar, T& t)
    {
        using cereal::CEREAL_LOAD_FUNCTION_NAME;
        CEREAL_LOAD_FUNCTION_NAME(ar.previous, t);
    }

    template <class CharT, class Traits, class Alloc>
    friend void
    CEREAL_LOAD_FUNCTION_NAME(json_immer_input_archive& ar,
                              std::basic_string<CharT, Traits, Alloc>& str)
    {
        using cereal::CEREAL_LOAD_FUNCTION_NAME;
        CEREAL_LOAD_FUNCTION_NAME(ar.previous, str);
    }

    template <class T>
    friend void CEREAL_LOAD_FUNCTION_NAME(json_immer_input_archive& ar,
                                          cereal::SizeTag<T>& st)
    {
        using cereal::CEREAL_LOAD_FUNCTION_NAME;
        CEREAL_LOAD_FUNCTION_NAME(ar.previous, st);
    }

    bool ignore_pool_exceptions = false;

private:
    WrapFn wrap;
    Previous previous;
    Pools pools;

    using Loaders = decltype(Pools::generate_loaders());
    Loaders loaders;
};

} // namespace immer::persist

// tie input and output archives together
namespace cereal {
namespace traits {
namespace detail {
template <class Previous, class Pools, class WrapFn, class PoolNameFn>
struct get_output_from_input<
    immer::persist::
        json_immer_input_archive<Previous, Pools, WrapFn, PoolNameFn>>
{
    using type = immer::persist::
        json_immer_output_archive<Previous, Pools, WrapFn, PoolNameFn>;
};
template <class Previous, class Pools, class WrapFn, class PoolNameFn>
struct get_input_from_output<
    immer::persist::
        json_immer_output_archive<Previous, Pools, WrapFn, PoolNameFn>>
{
    using type = immer::persist::
        json_immer_input_archive<Previous, Pools, WrapFn, PoolNameFn>;
};
} // namespace detail
} // namespace traits
} // namespace cereal
