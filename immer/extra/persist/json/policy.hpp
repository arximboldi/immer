#pragma once

#include <immer/extra/persist/json/names.hpp>
#include <immer/extra/persist/json/wrap.hpp>

namespace immer::persist {

template <class T, class Value>
concept Policy =
    requires(Value value, T policy) { policy.get_pool_types(value); };

template <class T>
auto get_pools_names(const T&)
{
    return boost::hana::make_map();
}

template <class T>
auto get_pools_types(const T&)
{
    return boost::hana::make_set();
}

struct value0_serialize_t
{
    template <class Archive, class T>
    void save(Archive& ar, const T& value0) const
    {
        ar(CEREAL_NVP(value0));
    }

    template <class Archive, class T>
    void load(Archive& ar, T& value0) const
    {
        ar(CEREAL_NVP(value0));
    }
};

template <class T>
struct via_get_pools_names_policy_t : value0_serialize_t
{
    auto get_pool_types(const T& value) const
    {
        return boost::hana::to_set(boost::hana::keys(get_pools_names(value)));
    }

    using Map = decltype(get_pools_names(std::declval<T>()));

    template <class Container>
    auto get_pool_name(const Container& container) const
    {
        return name_from_map_fn<Map>{}(container);
    }
};

auto via_get_pools_names_policy(const auto& value)
{
    return via_get_pools_names_policy_t<std::decay_t<decltype(value)>>{};
}

struct demangled_names_t
{
    template <class T>
    auto get_pool_name(const T& value) const
    {
        return get_demangled_name(value);
    }
};

struct via_get_pools_types_policy
    : demangled_names_t
    , value0_serialize_t
{
    template <class T>
    auto get_pool_types(const T& value) const
    {
        return get_pools_types(value);
    }
};

struct hana_struct_auto_policy : demangled_names_t
{
    template <class T>
    auto get_pool_types(const T& value) const
    {
        return get_pools_for_hana_type<T>();
    }
};

template <class T>
struct hana_struct_auto_member_name_policy_t : value0_serialize_t
{
    auto get_pool_types(const T& value) const
    {
        return get_pools_for_hana_type<T>();
    }

    using map_t = decltype(get_named_pools_for_hana_type<T>());

    template <class Container>
    auto get_pool_name(const Container& container) const
    {
        return name_from_map_fn<map_t>{}(container);
    }
};

auto hana_struct_auto_member_name_policy(const auto& value)
{
    return hana_struct_auto_member_name_policy_t<
        std::decay_t<decltype(value)>>{};
}

template <class Map>
struct via_map_policy : value0_serialize_t
{
    static_assert(boost::hana::is_a<boost::hana::map_tag, Map>,
                  "via_map_policy accepts a map of types to pool names");

    template <class T>
    auto get_pool_types(const T& value) const
    {
        return boost::hana::to_set(boost::hana::keys(Map{}));
    }

    template <class T>
    auto get_pool_name(const T& value) const
    {
        return name_from_map_fn<Map>{}(value);
    }
};

using default_policy = via_get_pools_types_policy;

} // namespace immer::persist
