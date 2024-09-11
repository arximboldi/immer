#pragma once

#include <immer/extra/persist/cereal/load.hpp>
#include <immer/extra/persist/cereal/save.hpp>
#include <immer/extra/persist/xxhash/xxhash.hpp>

#include <immer/table.hpp>

#include <sstream>

#include <cereal/archives/json.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/vector.hpp>

#include <fmt/format.h>
#include <fmt/ostream.h>

namespace test {

template <class T>
using vector_one =
    immer::vector<T, immer::default_memory_policy, immer::default_bits, 1>;

template <class T>
using flex_vector_one =
    immer::flex_vector<T, immer::default_memory_policy, immer::default_bits, 1>;

using example_vector      = vector_one<int>;
using example_flex_vector = flex_vector_one<int>;

using example_output_pool =
    decltype(immer::persist::rbts::make_output_pool_for(example_vector{}));
using example_loader = immer::persist::rbts::
    loader<int, immer::default_memory_policy, immer::default_bits, 1>;

template <class T>
auto gen(T init, int count)
{
    for (int i = 0; i < count; ++i) {
        init = std::move(init).push_back(i);
    }
    return init;
}

template <typename T>
std::string to_json(const T& serializable)
{
    auto os = std::ostringstream{};
    {
        auto ar = cereal::JSONOutputArchive{os};
        ar(serializable);
    }
    return os.str();
}

template <typename T>
T from_json(std::string input)
{
    auto is = std::istringstream{input};
    auto ar = cereal::JSONInputArchive{is};
    auto r  = T{};
    ar(r);
    return r;
}

struct test_value
{
    std::size_t id;
    std::string value;

    auto tie() const { return std::tie(id, value); }

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(id), CEREAL_NVP(value));
    }

    friend bool operator==(const test_value& left, const test_value& right)
    {
        return left.tie() == right.tie();
    }

    friend std::ostream& operator<<(std::ostream& s, const test_value& value)
    {
        return s << fmt::format("({}, {})", value.id, value.value);
    }
};

struct old_type
{
    std::string id;
    int data;

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(id), CEREAL_NVP(data));
    }

    auto tie() const { return std::tie(id, data); }

    friend bool operator==(const old_type& left, const old_type& right)
    {
        return left.tie() == right.tie();
    }
};

struct new_type
{
    std::string id;
    int data;
    std::string data2;

    auto tie() const { return std::tie(id, data, data2); }

    friend bool operator==(const new_type& left, const new_type& right)
    {
        return left.tie() == right.tie();
    }

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(id), CEREAL_NVP(data), CEREAL_NVP(data2));
    }
};

inline auto convert_old_type_impl(const old_type& val)
{
    return new_type{
        .id    = val.id,
        .data  = val.data,
        .data2 = fmt::format("_{}_", val.data),
    };
}

inline auto convert_old_type_impl(const std::pair<std::string, old_type>& val)
{
    return std::make_pair(val.first, convert_old_type_impl(val.second));
}

constexpr auto convert_old_type = [](auto&& arg) {
    return convert_old_type_impl(std::forward<decltype(arg)>(arg));
};

constexpr auto convert_old_type_map = boost::hana::overload(
    [](immer::persist::target_container_type_request) {
        return immer::
            map<std::string, new_type, immer::persist::xx_hash<std::string>>{};
    },
    convert_old_type);

constexpr auto convert_old_type_table = boost::hana::overload(
    [](immer::persist::target_container_type_request) {
        return immer::table<new_type,
                            immer::table_key_fn,
                            immer::persist::xx_hash<std::string>>{};
    },
    convert_old_type);

template <class T>
auto transform_vec(const T& vec)
{
    auto result = vector_one<new_type>{};
    for (const auto& item : vec) {
        result = std::move(result).push_back(convert_old_type(item));
    }
    return result;
}

template <class T>
auto transform_map(const T& map)
{
    auto result = immer::
        map<std::string, new_type, immer::persist::xx_hash<std::string>>{};
    for (const auto& [key, value] : map) {
        result = std::move(result).set(key, convert_old_type(value));
    }
    return result;
}

template <class T>
auto transform_table(const T& table)
{
    auto result = immer::table<new_type,
                               immer::table_key_fn,
                               immer::persist::xx_hash<std::string>>{};
    for (const auto& item : table) {
        result = std::move(result).insert(convert_old_type(item));
    }
    return result;
}

namespace hana = boost::hana;

/**
 * Unexpected thing: we can't use `hana::equal` to implement the operator== for
 * a struct. It ends up calling itself in an endless recursion.
 */
constexpr auto members = [](const auto& value) {
    return hana::keys(value) | [&value](auto key) {
        return hana::make_tuple(hana::at_key(value, key));
    };
};

constexpr auto serialize_members = [](auto& ar, auto& value) {
    hana::for_each(hana::keys(value), [&](auto key) {
        ar(cereal::make_nvp(key.c_str(), hana::at_key(value, key)));
    });
};

template <class Map>
struct via_map_policy : immer::persist::value0_serialize_t
{
    static_assert(boost::hana::is_a<boost::hana::map_tag, Map>,
                  "via_map_policy accepts a map of types to pool names");

    template <class T>
    auto get_pool_types(const T& value) const
    {
        return boost::hana::keys(Map{});
    }

    template <class T>
    auto get_pool_name(const T& value) const
    {
        return immer::persist::detail::name_from_map_fn<Map>{}(value);
    }
};

template <typename T, class PoolsTypes>
auto to_json_with_auto_pool(const T& serializable,
                            const PoolsTypes& pools_types)
{
    return immer::persist::cereal_save_with_pools(serializable,
                                                  via_map_policy<PoolsTypes>{});
}

template <typename T, class PoolsTypes>
T from_json_with_auto_pool(const std::string& input,
                           const PoolsTypes& pools_types)
{
    return immer::persist::cereal_load_with_pools<T>(
        input, via_map_policy<PoolsTypes>{});
}

} // namespace test

template <>
struct fmt::formatter<test::test_value> : ostream_formatter
{};
