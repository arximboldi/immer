#pragma once

#include <immer/extra/archive/rbts/load.hpp>
#include <immer/extra/archive/rbts/save.hpp>
#include <immer/extra/archive/traits.hpp>

namespace immer::archive {

template <typename T,
          typename MemoryPolicy,
          immer::detail::rbts::bits_t B,
          immer::detail::rbts::bits_t BL>
struct container_traits<immer::vector<T, MemoryPolicy, B, BL>>
{
    using save_archive_t = rbts::archive_save<T, MemoryPolicy, B, BL>;
    using load_archive_t = rbts::archive_load<T>;
    using container_id   = immer::archive::container_id;

    template <typename Archive    = load_archive_t,
              typename TransformF = boost::hana::id_t>
    using loader_t =
        rbts::vector_loader<T, MemoryPolicy, B, BL, Archive, TransformF>;

    // This function is used to determine the type of the container after
    // applying some transformation.
    template <class F>
    static auto transform(F&& func)
    {
        using U = std::decay_t<decltype(func(std::declval<T>()))>;
        return immer::vector<U, MemoryPolicy, B, BL>{};
    }
};

template <typename T,
          typename MemoryPolicy,
          immer::detail::rbts::bits_t B,
          immer::detail::rbts::bits_t BL>
struct container_traits<immer::flex_vector<T, MemoryPolicy, B, BL>>
{
    using save_archive_t = rbts::archive_save<T, MemoryPolicy, B, BL>;
    using load_archive_t = rbts::archive_load<T>;
    using container_id   = immer::archive::container_id;

    template <typename Archive    = load_archive_t,
              typename TransformF = boost::hana::id_t>
    using loader_t =
        rbts::flex_vector_loader<T, MemoryPolicy, B, BL, Archive, TransformF>;

    template <class F>
    static auto transform(F&& func)
    {
        using U = std::decay_t<decltype(func(std::declval<T>()))>;
        return immer::flex_vector<U, MemoryPolicy, B, BL>{};
    }
};

} // namespace immer::archive
