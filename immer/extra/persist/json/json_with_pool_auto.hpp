#pragma once

#include <immer/extra/persist/json/json_with_pool.hpp>

namespace immer::persist {

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
