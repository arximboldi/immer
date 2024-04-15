#pragma once

#include <immer/extra/archive/champ/archive.hpp>
#include <immer/extra/archive/champ/champ.hpp>
#include <immer/extra/archive/traits.hpp>

#include <boost/hana/functional/id.hpp>

namespace immer::archive {

template <class Container>
struct champ_traits
{
    using save_archive_t =
        immer::archive::champ::container_archive_save<Container>;
    using load_archive_t =
        immer::archive::champ::container_archive_load<Container>;
    using container_id = immer::archive::node_id;

    template <typename Archive    = load_archive_t,
              typename TransformF = boost::hana::id_t>
    using loader_t =
        immer::archive::champ::container_loader<Container, Archive, TransformF>;
};

/**
 * A bit of a hack but currently this is the simplest way to request a type of
 * the hash function to be used after the transformation. Maybe the whole thing
 * would change later. Right now everything is driven by the single function,
 * which seems to be convenient otherwise.
 */
struct target_container_type_request
{};

template <typename K,
          typename T,
          typename Hash,
          typename Equal,
          typename MemoryPolicy,
          immer::detail::hamts::bits_t B>
struct container_traits<immer::map<K, T, Hash, Equal, MemoryPolicy, B>>
    : champ_traits<immer::map<K, T, Hash, Equal, MemoryPolicy, B>>
{
    template <class F>
    static auto transform(F&& func)
    {
        // We need this special target_container_type_request because we can't
        // determine the hash and equality operators for the new key any other
        // way.
        using NewContainer =
            std::decay_t<decltype(func(target_container_type_request{}))>;
        return NewContainer{};
    }
};

template <typename T,
          typename KeyFn,
          typename Hash,
          typename Equal,
          typename MemoryPolicy,
          immer::detail::hamts::bits_t B>
struct container_traits<immer::table<T, KeyFn, Hash, Equal, MemoryPolicy, B>>
    : champ_traits<immer::table<T, KeyFn, Hash, Equal, MemoryPolicy, B>>
{
    template <class F>
    static auto transform(F&& func)
    {
        using U = std::decay_t<decltype(func(std::declval<T>()))>;
        return immer::table<U, KeyFn, Hash, Equal, MemoryPolicy, B>{};
    }
};

namespace champ {
template <typename K,
          typename T,
          typename Hash,
          typename Equal,
          typename MemoryPolicy,
          immer::detail::hamts::bits_t B,
          class F>
auto transform_archive(const container_archive_load<
                           immer::map<K, T, Hash, Equal, MemoryPolicy, B>>& ar,
                       F&& func)
{
    using old_map_t = immer::map<K, T, Hash, Equal, MemoryPolicy, B>;
    using new_map_t = decltype(container_traits<old_map_t>::transform(func));
    return container_archive_load<new_map_t>{
        .nodes = transform(ar.nodes, func),
    };
}

template <typename T,
          typename KeyFn,
          typename Hash,
          typename Equal,
          typename MemoryPolicy,
          immer::detail::hamts::bits_t B,
          class F>
auto transform_archive(
    const container_archive_load<
        immer::table<T, KeyFn, Hash, Equal, MemoryPolicy, B>>& ar,
    F&& func)
{
    using U           = std::decay_t<decltype(func(std::declval<T>()))>;
    using new_table_t = immer::table<U, KeyFn, Hash, Equal, MemoryPolicy, B>;
    return container_archive_load<new_table_t>{
        .nodes = transform(ar.nodes, func),
    };
}
} // namespace champ

} // namespace immer::archive
