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
        using U = std::decay_t<decltype(func(std::declval<T>()))>;
        return immer::map<K, U, Hash, Equal, MemoryPolicy, B>{};
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

} // namespace immer::archive
