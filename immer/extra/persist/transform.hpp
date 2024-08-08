#pragma once

#include <immer/extra/persist/detail/transform.hpp>

namespace immer::persist {

/**
 * Given an output_pools and a map of transformations, produce a new type of
 * load pool with those transformations applied
 */
template <class Storage, class ConversionMap>
inline auto
transform_output_pool(const detail::output_pools<Storage>& old_pools,
                      const ConversionMap& conversion_map)
{
    const auto old_load_pools = to_input_pools(old_pools);
    // NOTE: We have to copy old_pools here because the get_id function will
    // be called later, as the conversion process is lazy.
    const auto get_id = [old_pools](const auto& immer_container) {
        return detail::get_container_id(old_pools, immer_container);
    };
    return old_load_pools.transform_recursive(conversion_map, get_id);
}

/**
 * Given an old save pools and a new (transformed) load pools, effectively
 * convert the given container.
 */
template <class SaveStorage, class LoadStorage, class Container>
auto convert_container(const detail::output_pools<SaveStorage>& old_save_pools,
                       detail::input_pools<LoadStorage>& new_load_pools,
                       const Container& container)
{
    const auto container_id =
        detail::get_container_id(old_save_pools, container);
    auto& loader =
        new_load_pools
            .template get_loader_by_old_container<std::decay_t<Container>>();
    auto result = loader.load(container_id);
    return result;
}

} // namespace immer::persist
