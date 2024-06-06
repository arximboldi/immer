#pragma once

#include <immer/extra/persist/json/policy.hpp>
#include <immer/extra/persist/json/pools.hpp>

/**
 * to_json_with_pool
 */

namespace immer::persist {

/**
 * Type T must provide a callable free function get_pools_types(const T&).
 */
template <class Archive = cereal::JSONOutputArchive,
          class T,
          Policy<T> Policy = default_policy>
auto to_json_with_pool(const T& value0, const Policy& policy = Policy{})
{
    auto os = std::ostringstream{};
    {
        auto pools =
            detail::generate_output_pools(policy.get_pool_types(value0));
        using Pools = std::decay_t<decltype(pools)>;
        auto ar     = immer::persist::json_immer_output_archive<
            Archive,
            Pools,
            decltype(policy.get_output_wrap_fn()),
            decltype(policy.get_pool_name_fn(value0))>{
            pools, policy.get_output_wrap_fn(), os};
        policy.save(ar, value0);
    }
    return os.str();
}

template <class Pools, class Archive, class PoolNameFn>
auto load_pools(std::istream& is, const auto& wrap)
{
    const auto reload_pool = [wrap](std::istream& is,
                                    Pools pools,
                                    bool ignore_pool_exceptions) {
        auto restore              = immer::util::istream_snapshot{is};
        const auto original_pools = pools;
        auto ar =
            json_immer_input_archive<Archive,
                                     Pools,
                                     decltype(wrap),
                                     PoolNameFn>{std::move(pools), wrap, is};
        ar.ignore_pool_exceptions = ignore_pool_exceptions;
        /**
         * NOTE: Critical to clear the pools before loading into it
         * again. I hit a bug when pools contained a vector and every
         * load would append to it, instead of replacing the contents.
         */
        pools = {};
        ar(CEREAL_NVP(pools));
        pools.merge_previous(original_pools);
        return pools;
    };

    auto pools = Pools{};
    if constexpr (detail::is_pool_empty<Pools>()) {
        return pools;
    }

    auto prev = pools;
    while (true) {
        // Keep reloading until everything is loaded.
        // Reloading of the pool might trigger validation of some containers
        // (hash-based, for example) because the elements actually come from
        // other pools that are not yet loaded.
        constexpr bool ignore_pool_exceptions = true;
        pools = reload_pool(is, std::move(pools), ignore_pool_exceptions);
        if (prev == pools) {
            // Looks like we're done, reload one more time but do not ignore the
            // exceptions, for the final validation.
            pools = reload_pool(is, std::move(pools), false);
            break;
        }
        prev = pools;
    }

    return pools;
}

template <class T,
          class Archive    = cereal::JSONInputArchive,
          Policy<T> Policy = default_policy>
T from_json_with_pool(std::istream& is, const Policy& policy = Policy{})
{
    using Pools      = std::decay_t<decltype(detail::generate_input_pools(
        policy.get_pool_types(std::declval<T>())))>;
    using PoolNameFn = decltype(policy.get_pool_name_fn(std::declval<T>()));

    const auto& wrap = policy.get_input_wrap_fn();

    auto pools = load_pools<Pools, Archive, PoolNameFn>(is, wrap);

    auto ar = immer::persist::
        json_immer_input_archive<Archive, Pools, decltype(wrap), PoolNameFn>{
            std::move(pools), wrap, is};
    auto value0 = T{};
    policy.load(ar, value0);
    return value0;
}

template <class T,
          class Archive    = cereal::JSONInputArchive,
          Policy<T> Policy = default_policy>
T from_json_with_pool(const std::string& input, const Policy& policy = Policy{})
{
    auto is = std::istringstream{input};
    return from_json_with_pool<T, Archive>(is, policy);
}

template <class Storage, class Container>
auto get_container_id(const detail::output_pools<Storage>& pools,
                      const Container& container)
{
    const auto& old_pool =
        pools.template get_output_pool<std::decay_t<Container>>();
    const auto [new_pool, id] = add_to_pool(container, old_pool);
    if (!(new_pool == old_pool)) {
        throw std::logic_error{
            "Expecting that the container has already been persisted"};
    }
    return id;
}

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
        return get_container_id(old_pools, immer_container);
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
    const auto container_id = get_container_id(old_save_pools, container);
    auto& loader =
        new_load_pools
            .template get_loader_by_old_container<std::decay_t<Container>>();
    auto result = loader.load(container_id);
    return result;
}

} // namespace immer::persist
