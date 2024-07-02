#pragma once

#include <immer/extra/persist/champ/champ.hpp>
#include <immer/extra/persist/champ/pool.hpp>
#include <immer/extra/persist/traits.hpp>

#include <boost/hana/functional/id.hpp>

namespace immer::persist {

/**
 * A bit of a hack but currently this is the simplest way to request a type of
 * the hash function to be used after the transformation. Maybe the whole thing
 * would change later. Right now everything is driven by the single function,
 * which seems to be convenient otherwise.
 */
struct target_container_type_request
{};

template <class Container>
struct champ_traits
{
    using output_pool_t =
        immer::persist::champ::container_output_pool<Container>;
    using input_pool_t = immer::persist::champ::container_input_pool<Container>;
    using container_id = immer::persist::node_id;

    template <typename Pool       = input_pool_t,
              typename TransformF = boost::hana::id_t>
    using loader_t =
        immer::persist::champ::container_loader<Container, Pool, TransformF>;

    template <class F>
    static auto transform(F&& func)
        requires std::is_invocable_v<F, target_container_type_request>
    {
        // We need this special target_container_type_request because we can't
        // determine the hash and equality operators for the new key any other
        // way.
        using NewContainer =
            std::decay_t<decltype(func(target_container_type_request{}))>;
        return NewContainer{};
    }
};

template <typename K,
          typename T,
          typename Hash,
          typename Equal,
          typename MemoryPolicy,
          immer::detail::hamts::bits_t B>
struct container_traits<immer::map<K, T, Hash, Equal, MemoryPolicy, B>>
    : champ_traits<immer::map<K, T, Hash, Equal, MemoryPolicy, B>>
{};

template <typename T,
          typename KeyFn,
          typename Hash,
          typename Equal,
          typename MemoryPolicy,
          immer::detail::hamts::bits_t B>
struct container_traits<immer::table<T, KeyFn, Hash, Equal, MemoryPolicy, B>>
    : champ_traits<immer::table<T, KeyFn, Hash, Equal, MemoryPolicy, B>>
{};

template <typename T,
          typename Hash,
          typename Equal,
          typename MemoryPolicy,
          immer::detail::hamts::bits_t B>
struct container_traits<immer::set<T, Hash, Equal, MemoryPolicy, B>>
    : champ_traits<immer::set<T, Hash, Equal, MemoryPolicy, B>>
{};

namespace champ {
/**
 * The wrapper is used to enable the incompatible_hash_mode, which is required
 * when the key of a hash-based container transformed in a way that changes its
 * hash.
 */
template <class Container>
struct incompatible_hash_wrapper
{};
} // namespace champ

template <class Container>
struct container_traits<champ::incompatible_hash_wrapper<Container>>
    : champ_traits<Container>
{
    using base_t = champ_traits<Container>;

    // Everything stays the same as for normal container, except that we tell
    // the loader to do something special.
    static constexpr bool enable_incompatible_hash_mode = true;

    template <typename Pool       = typename base_t::input_pool_t,
              typename TransformF = boost::hana::id_t>
    using loader_t =
        immer::persist::champ::container_loader<Container,
                                                Pool,
                                                TransformF,
                                                enable_incompatible_hash_mode>;
};

} // namespace immer::persist
