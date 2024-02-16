#pragma once

#include <immer/experimental/immer-archive/rbts/load.hpp>
#include <immer/experimental/immer-archive/rbts/save.hpp>
#include <immer/experimental/immer-archive/traits.hpp>

namespace immer_archive {

template <typename T,
          typename MemoryPolicy,
          immer::detail::rbts::bits_t B,
          immer::detail::rbts::bits_t BL>
struct container_traits<immer::vector<T, MemoryPolicy, B, BL>>
{
    using save_archive_t = rbts::archive_save<T, MemoryPolicy, B, BL>;
    using load_archive_t = rbts::archive_load<T>;
    using loader_t       = rbts::vector_loader<T, MemoryPolicy, B, BL>;
    using container_id   = immer_archive::container_id;
};

template <typename T,
          typename MemoryPolicy,
          immer::detail::rbts::bits_t B,
          immer::detail::rbts::bits_t BL>
struct container_traits<immer::flex_vector<T, MemoryPolicy, B, BL>>
{
    using save_archive_t = rbts::archive_save<T, MemoryPolicy, B, BL>;
    using load_archive_t = rbts::archive_load<T>;
    using loader_t       = rbts::flex_vector_loader<T, MemoryPolicy, B, BL>;
    using container_id   = immer_archive::container_id;
};

} // namespace immer_archive
