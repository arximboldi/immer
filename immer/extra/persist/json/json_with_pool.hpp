#pragma once

#include <immer/extra/persist/detail/json/input_archive_util.hpp>
#include <immer/extra/persist/detail/json/pools.hpp>
#include <immer/extra/persist/detail/json/wrap.hpp>
#include <immer/extra/persist/json/policy.hpp>

namespace immer::persist {

/**
 * @defgroup persist-api
 */

/**
 * @brief Serialize the provided value using the provided policy to JSON
 * outputting into the provided stream.
 *
 * @see Policy
 * @ingroup persist-api
 */
template <class Archive = cereal::JSONOutputArchive,
          class T,
          Policy<T> Policy = default_policy>
void to_json_with_pool_stream(auto& os,
                              const T& value0,
                              const Policy& policy = Policy{})
{
    const auto types = policy.get_pool_types(value0);
    auto pools       = detail::generate_output_pools(types);
    const auto wrap  = detail::wrap_known_types(types, detail::wrap_for_saving);
    using Pools      = std::decay_t<decltype(pools)>;
    auto get_pool_name_fn = [](const auto& value) {
        return Policy{}.get_pool_name(value);
    };
    auto ar =
        immer::persist::json_immer_output_archive<Archive,
                                                  Pools,
                                                  decltype(wrap),
                                                  decltype(get_pool_name_fn)>{
            pools, wrap, os};
    policy.save(ar, value0);
    // Calling finalize explicitly, as it might throw on saving the pools,
    // for example if pool names are not unique.
    ar.finalize();
}

/**
 * @brief Serialize the provided value using the provided policy to JSON.
 *
 * @return std::string The resulting JSON.
 * @ingroup persist-api
 */
template <class Archive = cereal::JSONOutputArchive,
          class T,
          Policy<T> Policy = default_policy>
std::string to_json_with_pool(const T& value0, const Policy& policy = Policy{})
{
    auto os = std::ostringstream{};
    to_json_with_pool_stream<Archive>(os, value0, policy);
    return os.str();
}

template <class T,
          class Archive    = cereal::JSONInputArchive,
          Policy<T> Policy = default_policy>
T from_json_with_pool(std::istream& is, const Policy& policy = Policy{})
{
    using TypesSet = decltype(policy.get_pool_types(std::declval<T>()));
    using Pools    = decltype(detail::generate_input_pools(TypesSet{}));

    auto get_pool_name_fn = [](const auto& value) {
        return Policy{}.get_pool_name(value);
    };
    using PoolNameFn = decltype(get_pool_name_fn);

    const auto wrap =
        detail::wrap_known_types(TypesSet{}, detail::wrap_for_loading);
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

template <typename T, Policy<T> Policy = hana_struct_auto_policy>
auto get_auto_pool(const T& value0, const Policy& policy = Policy{})
{
    const auto types = policy.get_pool_types(value0);
    auto pools       = detail::generate_output_pools(types);
    const auto wrap  = detail::wrap_known_types(types, detail::wrap_for_saving);
    using Pools      = std::decay_t<decltype(pools)>;

    {
        auto ar = json_immer_output_archive<detail::blackhole_output_archive,
                                            Pools,
                                            decltype(wrap),
                                            detail::empty_name_fn>{pools, wrap};
        ar(CEREAL_NVP(value0));
        ar.finalize();
        pools = std::move(ar).get_output_pools();
    }
    return pools;
}

} // namespace immer::persist
