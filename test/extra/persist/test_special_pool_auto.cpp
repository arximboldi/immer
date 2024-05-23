#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "utils.hpp"

#include <immer/extra/persist/json/json_with_pool_auto.hpp>
#include <nlohmann/json.hpp>

#define DEFINE_OPERATIONS(name)                                                \
    bool operator==(const name& left, const name& right)                       \
    {                                                                          \
        return members(left) == members(right);                                \
    }                                                                          \
    template <class Archive>                                                   \
    void serialize(Archive& ar, name& m)                                       \
    {                                                                          \
        serialize_members(ar, m);                                              \
    }

namespace {

namespace hana = boost::hana;
using test::flex_vector_one;
using test::members;
using test::serialize_members;
using test::test_value;
using test::vector_one;

/**
 * A data type with immer members, note the absence of `persistable`.
 */
struct meta_meta
{
    BOOST_HANA_DEFINE_STRUCT(meta_meta,
                             (vector_one<int>, ints),
                             (immer::table<test_value>, table));

    friend bool operator==(const meta_meta& left, const meta_meta& right)
    {
        return members(left) == members(right);
    }

    template <class Archive>
    void serialize(Archive& ar)
    {
        serialize_members(ar, *this);
    }
};

struct meta
{
    BOOST_HANA_DEFINE_STRUCT(meta,
                             (vector_one<int>, ints),
                             (vector_one<meta_meta>, metas)

    );

    friend bool operator==(const meta& left, const meta& right)
    {
        return members(left) == members(right);
    }

    template <class Archive>
    void serialize(Archive& ar)
    {
        serialize_members(ar, *this);
    }
};

struct test_data_with_immer
{
    BOOST_HANA_DEFINE_STRUCT(
        test_data_with_immer,
        // Contains immer types
        (vector_one<int>, ints),
        (vector_one<std::string>, strings),
        (immer::map<int, std::string>, map),
        (vector_one<meta>, metas),
        // Map value is indirectly persistable
        (immer::map<int, meta>, metas_map),
        // Map value is directly persistable
        (immer::map<int, vector_one<int>>, vectors_map),
        // Also test having meta directly, not inside an persistable type
        (meta, single_meta),
        (immer::box<std::string>, box),

        // And non-immer types
        (std::vector<int>, std_vector_ints)

    );

    friend bool operator==(const test_data_with_immer& left,
                           const test_data_with_immer& right)
    {
        return members(left) == members(right);
    }

    /**
     * Serialization function is defined as normal.
     */
    template <class Archive>
    void serialize(Archive& ar)
    {
        serialize_members(ar, *this);
    }
};

} // namespace

TEST_CASE("Auto-persisting")
{
    constexpr auto names = [] {
        return immer::persist::get_pools_for_types(
            hana::tuple_t<test_data_with_immer, meta, meta_meta>,
            hana::make_map(hana::make_pair(hana::type_c<vector_one<meta_meta>>,
                                           BOOST_HANA_STRING("meta_meta"))));
    };

    using PoolTypes           = decltype(names());
    constexpr auto pool_types = PoolTypes{};

    // Verify auto-generated map of names and types
    static_assert(pool_types[hana::type_c<vector_one<int>>] ==
                  BOOST_HANA_STRING("ints"));
    static_assert(pool_types[hana::type_c<immer::map<int, std::string>>] ==
                  BOOST_HANA_STRING("map"));

    const auto ints1 = vector_one<int>{
        1,
        2,
    };
    const auto ints2      = ints1.push_back(3).push_back(4).push_back(5);
    const auto meta_value = meta{
        .ints = ints1,
        .metas =
            {
                meta_meta{
                    .ints  = ints1,
                    .table = {test_value{}},
                },
            },
    };
    const auto value = test_data_with_immer{
        .ints    = ints2,
        .strings = {"one", "two"},
        .map =
            {
                {1, "_one_"},
                {2, "two__"},
            },
        .metas =
            {
                meta_value,
                meta{
                    .ints = ints2,
                },
            },
        .metas_map =
            {
                {234, meta_value},
            },
        .vectors_map =
            {
                {234, {2, 3, 4}},
                {567, {5, 6, 7}},
                {789, ints1},
            },
        .single_meta     = meta_value,
        .box             = std::string{"hello from the box"},
        .std_vector_ints = {4, 5, 6, 7},
    };

    const auto [json_str, pools] =
        immer::persist::to_json_with_auto_pool(value, pool_types);
    // REQUIRE(json_str == "");

    {
        const auto loaded =
            immer::persist::from_json_with_auto_pool<test_data_with_immer>(
                json_str, pool_types);
        REQUIRE(loaded == value);
    }
}

TEST_CASE("Auto-pool must load and save types that have no pool")
{
    const auto val1  = test_value{123, "value1"};
    const auto val2  = test_value{234, "value2"};
    const auto value = std::make_pair(val1, val2);

    const auto json_pool_str =
        immer::persist::to_json_with_auto_pool(value, hana::make_map()).first;
    REQUIRE(json_pool_str == test::to_json(value));

    {
        auto loaded = immer::persist::from_json_with_auto_pool<
            std::decay_t<decltype(value)>>(json_pool_str, hana::make_map());
        INFO(loaded.first);
        INFO(loaded.second);
        INFO(value.first);
        INFO(value.second);
        REQUIRE(loaded == value);
    }
}

namespace {
// Just a small type for testing
struct test_data_with_one_immer_member
{
    BOOST_HANA_DEFINE_STRUCT(test_data_with_one_immer_member,
                             (vector_one<int>, ints)

    );

    friend bool operator==(const test_data_with_one_immer_member& left,
                           const test_data_with_one_immer_member& right)
    {
        return members(left) == members(right);
    }

    template <class Archive>
    void serialize(Archive& ar)
    {
        serialize_members(ar, *this);
    }
};
} // namespace

TEST_CASE("Test save and load small type")
{
    const auto ints1 = vector_one<int>{
        1,
        2,
    };
    const auto value = test_data_with_one_immer_member{
        .ints = ints1,
    };
    const auto pool_types = immer::persist::get_auto_pools_types(value);
    const auto [json_str, pools] =
        immer::persist::to_json_with_auto_pool(value, pool_types);
    // REQUIRE(json_str == "");

    {
        const auto loaded = immer::persist::from_json_with_auto_pool<
            test_data_with_one_immer_member>(json_str, pool_types);
        INFO(test::to_json(loaded));
        INFO(test::to_json(value));
        REQUIRE(loaded == value);
    }
}

namespace {

using test::new_type;
using test::old_type;

template <class V>
using map_t = immer::map<std::string, V, immer::persist::xx_hash<std::string>>;

template <class T>
using table_t =
    immer::table<T, immer::table_key_fn, immer::persist::xx_hash<std::string>>;

// Some type that an application would serialize. Contains multiple vectors and
// maps to demonstrate structural sharing.
struct old_app_type
{
    BOOST_HANA_DEFINE_STRUCT(old_app_type,
                             (test::vector_one<old_type>, vec),
                             (test::vector_one<old_type>, vec2),
                             (map_t<old_type>, map),
                             (map_t<old_type>, map2),
                             (table_t<old_type>, table)

    );

    template <class Archive>
    void serialize(Archive& ar)
    {
        serialize_members(ar, *this);
    }
};

struct new_app_type
{
    BOOST_HANA_DEFINE_STRUCT(new_app_type,
                             (test::vector_one<new_type>, vec),
                             (test::vector_one<new_type>, vec2),
                             (map_t<new_type>, map),
                             (map_t<new_type>, map2),
                             (table_t<new_type>, table)

    );

    template <class Archive>
    void serialize(Archive& ar)
    {
        serialize_members(ar, *this);
    }
};

} // namespace

TEST_CASE("Test conversion with auto-pool")
{
    const auto vec1 = test::vector_one<old_type>{
        old_type{.data = 123},
        old_type{.data = 234},
    };
    const auto vec2 = vec1.push_back(old_type{.data = 345});

    const auto map1 = [] {
        auto map = map_t<old_type>{};
        for (auto i = 0; i < 30; ++i) {
            map =
                std::move(map).set(fmt::format("x{}x", i), old_type{.data = i});
        }
        return map;
    }();
    const auto map2 = map1.set("345", old_type{.data = 345});

    // Prepare a value of the old type that uses some structural sharing
    // internally.
    const auto value = old_app_type{
        .vec  = vec1,
        .vec2 = vec2,
        .map  = map1,
        .map2 = map2,
        .table =
            {
                old_type{"_51_", 51},
                old_type{"_52_", 52},
                old_type{"_53_", 53},
            },
    };

    constexpr auto old_names = [] {
        return immer::persist::get_pools_for_types(hana::tuple_t<old_app_type>,
                                                   hana::make_map());
    };

    using OldPoolTypes            = decltype(old_names());
    constexpr auto old_pool_types = OldPoolTypes{};
    const auto [json_str, pools] =
        immer::persist::to_json_with_auto_pool(value, old_pool_types);
    // REQUIRE(json_str == "");

    // Describe how to go from the old pool to the desired new pool.
    // Convert all old pools with convert_old_type.
    const auto pools_conversions =
        hana::make_map(hana::make_pair(hana::type_c<test::vector_one<old_type>>,
                                       test::convert_old_type),
                       hana::make_pair(hana::type_c<map_t<old_type>>,
                                       test::convert_old_type_map),
                       hana::make_pair(hana::type_c<table_t<old_type>>,
                                       test::convert_old_type_table));

    // Having a JSON from serializing old_app_type and a conversion function,
    // we need to somehow load new_app_type.
    const new_app_type full_load =
        immer::persist::from_json_with_auto_pool_with_conversion<new_app_type,
                                                                 old_app_type>(
            json_str, pools_conversions, old_pool_types);

    {
        REQUIRE(full_load.vec == transform_vec(value.vec));
        REQUIRE(full_load.vec2 == transform_vec(value.vec2));
        REQUIRE(full_load.map == transform_map(value.map));
        REQUIRE(full_load.map2 == transform_map(value.map2));
        REQUIRE(full_load.table == transform_table(value.table));
    }
}

namespace champ_test {

struct value_two;

struct two_boxed
{
    BOOST_HANA_DEFINE_STRUCT(two_boxed, (immer::box<value_two>, two));

    two_boxed() = default;
    explicit two_boxed(value_two val);
};

struct key
{
    BOOST_HANA_DEFINE_STRUCT(key, (std::string, str));

    friend std::size_t xx_hash_value(const key& value)
    {
        return immer::persist::xx_hash_value_string(value.str);
    }
};

const key& get_table_key(const two_boxed& two);

struct table_key_fn
{
    const key& operator()(const two_boxed& two) const;

    template <typename T, typename K>
    auto operator()(T&& x, K&& k) const
    {
        return set_table_key(std::forward<T>(x), std::forward<K>(k));
    }
};

struct value_one
{
    BOOST_HANA_DEFINE_STRUCT(
        value_one, //
        (vector_one<two_boxed>, twos),
        (immer::table<two_boxed, table_key_fn, immer::persist::xx_hash<key>>,
         twos_table));
};

struct value_two
{
    vector_one<value_one> ones = {};
    key key                    = {};

    friend std::ostream& operator<<(std::ostream& s, const value_two& value)
    {
        return s << fmt::format(
                   "ones = {}, key = '{}'", value.ones.size(), value.key.str);
    }
};

const key& table_key_fn::operator()(const two_boxed& two) const
{
    return two.two.get().key;
}

} // namespace champ_test

template <>
struct fmt::formatter<champ_test::value_two> : ostream_formatter
{};

namespace champ_test {

std::ostream& operator<<(std::ostream& s, const two_boxed& value)
{
    return s << fmt::format("two_boxed[{}]", value.two.get());
}

const key& get_table_key(const two_boxed& two) { return two.two.get().key; }

std::size_t xx_hash_value(const two_boxed& value)
{
    return xx_hash_value(value.two.get().key);
}

two_boxed::two_boxed(value_two val)
    : two{val}
{
}

} // namespace champ_test

template <>
struct fmt::formatter<champ_test::two_boxed> : ostream_formatter
{};

BOOST_HANA_ADAPT_STRUCT(champ_test::value_two, ones, key);

namespace champ_test {
DEFINE_OPERATIONS(two_boxed);
DEFINE_OPERATIONS(key);
DEFINE_OPERATIONS(value_one);
DEFINE_OPERATIONS(value_two);
} // namespace champ_test

namespace {
struct project_value_ptr
{
    template <class T>
    const T* operator()(const T& v) const noexcept
    {
        return std::addressof(v);
    }
};
} // namespace

TEST_CASE("Test table with a funny value")
{
    const auto two1 = champ_test::two_boxed{champ_test::value_two{
        .key = champ_test::key{"456"},
    }};
    const auto t1 =
        immer::table<champ_test::two_boxed,
                     champ_test::table_key_fn,
                     immer::persist::xx_hash<champ_test::key>>{two1};
    const auto two2 = champ_test::two_boxed{champ_test::value_two{
        .ones =
            {
                champ_test::value_one{
                    .twos_table = t1,
                },
            },
        .key = champ_test::key{"123"},
    }};

    const auto value = champ_test::value_one{
        .twos_table = t1.insert(two2),
    };

    const auto names = immer::persist::get_pools_for_types(
        hana::tuple_t<champ_test::value_one,
                      champ_test::value_two,
                      champ_test::two_boxed>,
        hana::make_map());

    const auto [json_str, ar] =
        immer::persist::to_json_with_auto_pool(value, names);
    // REQUIRE(json_str == "");

    const auto loaded =
        immer::persist::from_json_with_auto_pool<champ_test::value_one>(
            json_str, names);
    REQUIRE(loaded == value);
}

TEST_CASE("Test loading broken table")
{
    const auto two1 = champ_test::two_boxed{champ_test::value_two{
        .key = champ_test::key{"456"},
    }};
    const auto t1 =
        immer::table<champ_test::two_boxed,
                     champ_test::table_key_fn,
                     immer::persist::xx_hash<champ_test::key>>{two1};
    const auto two2 = champ_test::two_boxed{champ_test::value_two{
        .ones =
            {
                champ_test::value_one{
                    .twos_table = t1,
                },
            },
        .key = champ_test::key{"123"},
    }};

    const auto value = champ_test::value_one{
        .twos       = {champ_test::two_boxed{champ_test::value_two{
                           .key = champ_test::key{"90"},
                 }},
                       champ_test::two_boxed{champ_test::value_two{
                           .key = champ_test::key{"91"},
                 }}},
        .twos_table = t1.insert(two2),
    };

    const auto names = immer::persist::get_pools_for_types(
        hana::tuple_t<champ_test::value_one,
                      champ_test::value_two,
                      champ_test::two_boxed>,
        hana::make_map());

    const auto [json_str, ar] =
        immer::persist::to_json_with_auto_pool(value, names);
    // REQUIRE(json_str == "");

    constexpr auto expected_json_str = R"(
{
  "value0": {"twos": 0, "twos_table": 0},
  "pools": {
    "two": [
      {"ones": 0, "key": {"str": "90"}},
      {"ones": 0, "key": {"str": "91"}},
      {"ones": 0, "key": {"str": "456"}},
      {"ones": 1, "key": {"str": "123"}}
    ],
    "ones": {
      "leaves": [
        {"key": 1, "value": []},
        {"key": 2, "value": [{"twos": 1, "twos_table": 1}]}
      ],
      "inners": [{"key": 0, "value": {"children": [], "relaxed": false}}],
      "vectors": [{"root": 0, "tail": 1}, {"root": 0, "tail": 2}]
    },
    "twos": {
      "leaves": [
        {"key": 1, "value": [{"two": 0}, {"two": 1}]},
        {"key": 2, "value": []}
      ],
      "inners": [{"key": 0, "value": {"children": [], "relaxed": false}}],
      "vectors": [{"root": 0, "tail": 1}, {"root": 0, "tail": 2}]
    },
    "twos_table": [
      {
        "values": [{"two": 2}, {"two": 3}],
        "children": [],
        "nodemap": 0,
        "datamap": 4100,
        "collisions": false
      },
      {
        "values": [{"two": 2}],
        "children": [],
        "nodemap": 0,
        "datamap": 4096,
        "collisions": false
      }
    ]
  }
})";
    using json_t                     = nlohmann::json;
    auto json                        = json_t::parse(expected_json_str);

    // Serialized json should look like this
    REQUIRE(json == json_t::parse(json_str));

    SECTION("Loads correctly 1")
    {
        INFO(json_str);
        const auto loaded =
            immer::persist::from_json_with_auto_pool<champ_test::value_one>(
                json_str, names);
        REQUIRE(loaded == value);
    }

    SECTION("Loads correctly 2")
    {
        INFO(json.dump());
        const auto loaded =
            immer::persist::from_json_with_auto_pool<champ_test::value_one>(
                json.dump(), names);
        REQUIRE(loaded == value);
    }

    SECTION("Break the table that's part of the pool itself, not the loaded "
            "value")
    {
        SECTION("box exists")
        {
            json["pools"]["twos_table"][1]["values"][0]["two"] = 0;
            REQUIRE_THROWS_MATCHES(
                immer::persist::from_json_with_auto_pool<champ_test::value_one>(
                    json.dump(), names),
                ::cereal::Exception,
                MessageMatches(Catch::Matchers::ContainsSubstring(
                    "Couldn't find an element")));
        }
        SECTION("box doesn't exist")
        {
            json["pools"]["twos_table"][1]["values"][0]["two"] = 99;
            REQUIRE_THROWS_MATCHES(
                immer::persist::from_json_with_auto_pool<champ_test::value_one>(
                    json.dump(), names),
                ::cereal::Exception,
                MessageMatches(Catch::Matchers::ContainsSubstring(
                    "Couldn't find an element")));
        }
    }

    SECTION("Box that doesn't exist is referenced in the table for the value")
    {
        json["pools"]["twos_table"][0]["values"][0]["two"] = 99;
        REQUIRE_THROWS_MATCHES(
            immer::persist::from_json_with_auto_pool<champ_test::value_one>(
                json.dump(), names),
            ::cereal::Exception,
            MessageMatches(Catch::Matchers::ContainsSubstring(
                "Container ID 99 is not found")));
    }
}

namespace test_no_auto {

using immer::persist::persistable;

struct value_two;

struct two_boxed
{
    BOOST_HANA_DEFINE_STRUCT(two_boxed,
                             (persistable<immer::box<value_two>>, two));

    two_boxed() = default;
    explicit two_boxed(value_two val);
};

struct key
{
    BOOST_HANA_DEFINE_STRUCT(key, (std::string, str));

    friend std::size_t xx_hash_value(const key& value)
    {
        return immer::persist::xx_hash_value_string(value.str);
    }
};

const key& get_table_key(const two_boxed& two);

struct table_key_fn
{
    const key& operator()(const two_boxed& two) const;

    template <typename T, typename K>
    auto operator()(T&& x, K&& k) const
    {
        return set_table_key(std::forward<T>(x), std::forward<K>(k));
    }
};

struct value_one
{
    BOOST_HANA_DEFINE_STRUCT(
        value_one, //
        (vector_one<two_boxed>, twos),
        (persistable<immer::table<two_boxed,
                                  table_key_fn,
                                  immer::persist::xx_hash<key>>>,
         twos_table));
};

struct value_two
{
    vector_one<value_one> ones = {};
    key key                    = {};

    friend std::ostream& operator<<(std::ostream& s, const value_two& value)
    {
        return s << fmt::format(
                   "ones = {}, key = '{}'", value.ones.size(), value.key.str);
    }
};

const key& table_key_fn::operator()(const two_boxed& two) const
{
    return two.two.container.get().key;
}

} // namespace test_no_auto

template <>
struct fmt::formatter<test_no_auto::value_two> : ostream_formatter
{};

namespace test_no_auto {

std::ostream& operator<<(std::ostream& s, const two_boxed& value)
{
    return s << fmt::format("two_boxed[{}]", value.two.container.get());
}

const key& get_table_key(const two_boxed& two)
{
    return two.two.container.get().key;
}

std::size_t xx_hash_value(const two_boxed& value)
{
    return xx_hash_value(value.two.container.get().key);
}

two_boxed::two_boxed(value_two val)
    : two(immer::box<value_two>{val})
{
}

} // namespace test_no_auto

template <>
struct fmt::formatter<test_no_auto::two_boxed> : ostream_formatter
{};

BOOST_HANA_ADAPT_STRUCT(test_no_auto::value_two, ones, key);

namespace test_no_auto {
DEFINE_OPERATIONS(two_boxed);
DEFINE_OPERATIONS(key);
DEFINE_OPERATIONS(value_one);
DEFINE_OPERATIONS(value_two);

auto get_pools_types(const value_one&)
{
    return hana::make_map(
        hana::make_pair(hana::type_c<immer::box<value_two>>,
                        BOOST_HANA_STRING("box")),
        hana::make_pair(
            hana::type_c<immer::table<two_boxed,
                                      table_key_fn,
                                      immer::persist::xx_hash<key>>>,
            BOOST_HANA_STRING("table")));
}
} // namespace test_no_auto

TEST_CASE("Test table with a funny value no auto")
{
    const auto two1 = test_no_auto::two_boxed{test_no_auto::value_two{
        .key = test_no_auto::key{"456"},
    }};
    const auto t1 =
        immer::table<test_no_auto::two_boxed,
                     test_no_auto::table_key_fn,
                     immer::persist::xx_hash<test_no_auto::key>>{two1};
    const auto two2 = test_no_auto::two_boxed{test_no_auto::value_two{
        .ones =
            {
                test_no_auto::value_one{
                    .twos_table = t1,
                },
            },
        .key = test_no_auto::key{"123"},
    }};

    const auto value = test_no_auto::value_one{
        .twos_table = t1.insert(two2),
    };

    const auto [json_str, ar] = immer::persist::to_json_with_pool(value);
    // REQUIRE(json_str == "");

    const auto loaded =
        immer::persist::from_json_with_pool<test_no_auto::value_one>(json_str);
    REQUIRE(loaded == value);
}

namespace {

struct int_key
{
    BOOST_HANA_DEFINE_STRUCT(int_key, (int, id));
};
DEFINE_OPERATIONS(int_key);

struct string_key
{
    BOOST_HANA_DEFINE_STRUCT(string_key, (std::string, id));
};
DEFINE_OPERATIONS(string_key);

struct test_champs
{
    BOOST_HANA_DEFINE_STRUCT(test_champs,
                             (immer::map<int, std::string>, map),
                             (immer::table<int_key>, table),
                             (immer::set<int>, set)

    );
};
DEFINE_OPERATIONS(test_champs);

} // namespace

TEST_CASE("Structure breaks when hash is changed")
{
    const auto value = test_champs{
        .map = {{123, "123"}, {456, "456"}},
    };

    const auto names = immer::persist::get_pools_for_types(
        hana::tuple_t<test_champs>, hana::make_map());

    const auto [json_str, ar] =
        immer::persist::to_json_with_auto_pool(value, names);
    // REQUIRE(json_str == "");

    constexpr auto convert_pair = [](const std::pair<int, std::string>& old) {
        return std::make_pair(fmt::format("_{}_", old.first), old.second);
    };

    const auto map = hana::make_map(hana::make_pair(
        hana::type_c<immer::map<int, std::string>>,
        hana::overload(convert_pair,
                       [](immer::persist::target_container_type_request) {
                           // We just return the desired new type, but the hash
                           // of int is not compatible with the hash of string.
                           return immer::map<std::string, std::string>{};
                       }))

    );

    auto load_ar = immer::persist::transform_output_pool(ar, map);

    REQUIRE_THROWS_AS(immer::persist::convert_container(ar, load_ar, value.map),
                      immer::persist::champ::hash_validation_failed_exception);
}

TEST_CASE("Converting between incompatible keys")
{
    const auto value = test_champs{
        .map   = {{123, "123"}, {456, "456"}},
        .table = {{901}, {902}},
    };

    const auto names = immer::persist::get_pools_for_types(
        hana::tuple_t<test_champs>, hana::make_map());

    const auto ar = immer::persist::get_auto_pool(value, names);

    constexpr auto convert_pair = [](const std::pair<int, std::string>& old) {
        return std::make_pair(fmt::format("_{}_", old.first), old.second);
    };

    constexpr auto convert_int_key = [](const int_key& old) {
        return string_key{fmt::format("x{}x", old.id)};
    };

    /**
     * The problem is that the new key of the map has a completely different
     * hash from the old key, which makes the whole map structure unusable. We
     * need to have some special mode that essentially rebuilds the map. We will
     * lose all internal structural sharing but at least the same container_id
     * must return the same container (root node sharing).
     */
    const auto map = hana::make_map(
        hana::make_pair(
            hana::type_c<immer::map<int, std::string>>,
            hana::overload(
                convert_pair,
                [](immer::persist::target_container_type_request) {
                    return immer::persist::champ::incompatible_hash_wrapper<
                        immer::map<std::string, std::string>>{};
                })),
        hana::make_pair(
            hana::type_c<immer::table<int_key>>,
            hana::overload(
                convert_int_key,
                [](immer::persist::target_container_type_request) {
                    return immer::persist::champ::incompatible_hash_wrapper<
                        immer::table<string_key>>{};
                })),
        hana::make_pair(
            hana::type_c<immer::set<int>>,
            hana::overload(
                [convert_int_key](int old) {
                    return convert_int_key(int_key{old}).id;
                },
                [](immer::persist::target_container_type_request) {
                    return immer::persist::champ::incompatible_hash_wrapper<
                        immer::set<std::string>>{};
                }))

    );

    auto load_ar = immer::persist::transform_output_pool(ar, map);
    SECTION("maps")
    {
        constexpr auto convert_map = [convert_pair](const auto& map) {
            auto result = immer::map<std::string, std::string>{};
            for (const auto& item : map) {
                result = std::move(result).insert(convert_pair(item));
            }
            return result;
        };

        const auto converted =
            immer::persist::convert_container(ar, load_ar, value.map);
        REQUIRE(converted == convert_map(value.map));

        // Converting the same thing should return the same data
        const auto converted_2 =
            immer::persist::convert_container(ar, load_ar, value.map);
        REQUIRE(converted.identity() == converted_2.identity());
    }
    SECTION("tables")
    {
        constexpr auto convert_table = [convert_int_key](const auto& table) {
            auto result = immer::table<string_key>{};
            for (const auto& item : table) {
                result = std::move(result).insert(convert_int_key(item));
            }
            return result;
        };

        const auto converted =
            immer::persist::convert_container(ar, load_ar, value.table);
        REQUIRE(converted == convert_table(value.table));

        // Converting the same thing should return the same data
        const auto converted_2 =
            immer::persist::convert_container(ar, load_ar, value.table);
        REQUIRE(converted.impl().root == converted_2.impl().root);
    }
    SECTION("sets")
    {
        constexpr auto convert_set = [convert_int_key](const auto& set) {
            auto result = immer::set<std::string>{};
            for (const auto& item : set) {
                result =
                    std::move(result).insert(convert_int_key(int_key{item}).id);
            }
            return result;
        };

        const auto converted =
            immer::persist::convert_container(ar, load_ar, value.set);
        REQUIRE(converted == convert_set(value.set));

        // Converting the same thing should return the same data
        const auto converted_2 =
            immer::persist::convert_container(ar, load_ar, value.set);
        REQUIRE(converted.impl().root == converted_2.impl().root);
    }
}