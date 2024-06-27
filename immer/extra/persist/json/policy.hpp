#pragma once

#include <immer/extra/persist/json/names.hpp>
#include <immer/extra/persist/json/wrap.hpp>

namespace immer::persist {

template <class T, class Value>
concept Policy = requires(Value value, T policy) {
    policy.get_pool_types(value);
    policy.get_pool_name_fn(value);
};

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

struct via_get_pools_names_policy : value0_serialize_t
{
    template <class T>
    auto get_pool_types(const T& value) const
    {
        return boost::hana::to_set(boost::hana::keys(get_pools_names(value)));
    }

    template <class T>
    auto get_pool_name_fn(const T& value) const
    {
        using Map = decltype(get_pools_names(value));
        return name_from_map_fn<Map>{};
    }
};

struct demangled_names_t
{
    template <class T>
    auto get_pool_name_fn(const T& value) const
    {
        return get_demangled_name_fn{};
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
        return get_pools_for_type<T>();
    }
};

struct hana_struct_auto_member_name_policy : value0_serialize_t
{
    template <class T>
    auto get_pool_types(const T& value) const
    {
        return get_pools_for_type<T>();
    }

    template <class T>
    auto get_pool_name_fn(const T& value) const
    {
        using map_t = decltype(get_named_pools_for_type<T>());
        return name_from_map_fn<map_t>{};
    }
};

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
    auto get_pool_name_fn(const T&) const
    {
        return name_from_map_fn<Map>{};
    }
};

using default_policy = via_get_pools_types_policy;

} // namespace immer::persist
