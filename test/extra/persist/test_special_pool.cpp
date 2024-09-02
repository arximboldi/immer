#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "utils.hpp"

#include <boost/hana.hpp>
#include <immer/extra/persist/cereal/save.hpp>
#include <immer/extra/persist/xxhash/xxhash.hpp>

// to save std::pair
#include <cereal/types/utility.hpp>

#include <boost/hana/ext/std/tuple.hpp>

#include <nlohmann/json.hpp>

namespace {

namespace hana = boost::hana;

/**
 * Some user data type that contains some vector_one_persistable, which should
 * be serialized in a special way.
 */

using test::flex_vector_one;
using test::test_value;
using test::vector_one;
using json_t = nlohmann::json;

template <class T>
using per = immer::persist::detail::persistable<T>;

template <class T>
std::string string_via_tie(const T& value)
{
    std::string result;
    hana::for_each(value.tie(), [&](const auto& item) {
        using Item = std::decay_t<decltype(item)>;
        result += (result.empty() ? "" : ", ") +
                  Catch::StringMaker<Item>::convert(item);
    });
    return result;
}

struct meta_meta
{
    per<vector_one<int>> ints;
    per<immer::table<test_value>> table;

    auto tie() const { return std::tie(ints, table); }

    friend bool operator==(const meta_meta& left, const meta_meta& right)
    {
        return left.tie() == right.tie();
    }

    friend std::ostream& operator<<(std::ostream& s, const meta_meta& value)
    {
        return s << string_via_tie(value);
    }

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(ints), CEREAL_NVP(table));
    }
};

struct meta
{
    per<vector_one<int>> ints;
    per<vector_one<meta_meta>> metas;

    auto tie() const { return std::tie(ints, metas); }

    friend bool operator==(const meta& left, const meta& right)
    {
        return left.tie() == right.tie();
    }

    friend std::ostream& operator<<(std::ostream& s, const meta& value)
    {
        return s << string_via_tie(value);
    }

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(ints), CEREAL_NVP(metas));
    }
};

struct test_data
{
    per<vector_one<int>> ints;
    per<vector_one<std::string>> strings;

    per<flex_vector_one<int>> flex_ints;
    per<immer::map<int, std::string>> map;

    per<vector_one<meta>> metas;

    // Map value is indirectly persistable
    per<immer::map<int, meta>> metas_map;

    // Map value is directly persistable
    per<immer::map<int, per<vector_one<int>>>> vectors_map;

    // Also test having meta directly, not inside an persistable type
    meta single_meta;

    per<immer::box<std::string>> box;

    auto tie() const
    {
        return std::tie(ints,
                        strings,
                        flex_ints,
                        map,
                        metas,
                        metas_map,
                        vectors_map,
                        single_meta,
                        box);
    }

    friend bool operator==(const test_data& left, const test_data& right)
    {
        return left.tie() == right.tie();
    }

    /**
     * Serialization function is defined as normal.
     */
    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(ints),
           CEREAL_NVP(strings),
           CEREAL_NVP(flex_ints),
           CEREAL_NVP(map),
           CEREAL_NVP(metas),
           CEREAL_NVP(metas_map),
           CEREAL_NVP(vectors_map),
           CEREAL_NVP(single_meta),
           CEREAL_NVP(box));
    }
};

/**
 * A special function that enumerates which types of pools are
 * required. Explicitly name each type, for simplicity.
 */
inline auto get_pools_names(const test_data&)
{
    auto names = hana::make_map(
        hana::make_pair(hana::type_c<vector_one<int>>,
                        BOOST_HANA_STRING("ints")),
        hana::make_pair(hana::type_c<vector_one<std::string>>,
                        BOOST_HANA_STRING("strings")),
        hana::make_pair(hana::type_c<flex_vector_one<int>>,
                        BOOST_HANA_STRING("flex_ints")),
        hana::make_pair(hana::type_c<immer::map<int, std::string>>,
                        BOOST_HANA_STRING("int_string_map")),
        hana::make_pair(hana::type_c<vector_one<meta>>,
                        BOOST_HANA_STRING("metas")),
        hana::make_pair(hana::type_c<vector_one<meta_meta>>,
                        BOOST_HANA_STRING("meta_metas")),
        hana::make_pair(hana::type_c<immer::table<test_value>>,
                        BOOST_HANA_STRING("table_test_value")),
        hana::make_pair(hana::type_c<immer::map<int, meta>>,
                        BOOST_HANA_STRING("int_meta_map")),
        hana::make_pair(hana::type_c<immer::map<int, per<vector_one<int>>>>,
                        BOOST_HANA_STRING("int_vector_map")),
        hana::make_pair(hana::type_c<immer::box<std::string>>,
                        BOOST_HANA_STRING("string_box"))

    );
    return names;
}

auto get_pools_types(const test_data& val)
{
    return hana::keys(get_pools_names(val));
}

inline auto get_pools_names(const std::pair<test_data, test_data>&)
{
    return get_pools_names(test_data{});
}

auto get_pools_types(const std::pair<test_data, test_data>& val)
{
    return hana::keys(get_pools_names(val));
}

} // namespace

template <>
struct fmt::formatter<meta_meta> : ostream_formatter
{};

template <>
struct fmt::formatter<meta> : ostream_formatter
{};

namespace Catch {
template <>
struct StringMaker<test_data>
{
    static std::string convert(const test_data& value)
    {
        return string_via_tie(value);
    }
};

template <>
struct StringMaker<meta>
{
    static std::string convert(const meta& value)
    {
        return string_via_tie(value);
    }
};

template <>
struct StringMaker<meta_meta>
{
    static std::string convert(const meta_meta& value)
    {
        return string_via_tie(value);
    }
};
} // namespace Catch

TEST_CASE("Special pool minimal test")
{
    const auto ints1 = vector_one<int>{
        1,
        2,
        3,
        4,
        5,
    };
    const auto meta_value = meta{
        .ints = ints1,
        .metas =
            {
                meta_meta{
                    .ints = ints1,
                },
            },
    };
    const auto test1 = test_data{
        .metas =
            {
                meta_value,
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
    };

    const auto json_str = immer::persist::cereal_save_with_pools(
        test1,
        immer::persist::default_policy{},
        cereal::JSONOutputArchive::Options::NoIndent());
    // REQUIRE(json_str == "");

    {
        auto full_load =
            immer::persist::cereal_load_with_pools<test_data>(json_str);
        REQUIRE(full_load == test1);
        // REQUIRE(immer::persist::cereal_save_with_pools(full_load).first ==
        // "");
    }
}

TEST_CASE("Save with a special pool")
{
    const auto ints1 = test::gen(test::example_vector{}, 3);
    const auto test1 = test_data{
        .ints      = ints1,
        .strings   = {"one", "two"},
        .flex_ints = flex_vector_one<int>{ints1},
        .map =
            {
                {1, "_one_"},
                {2, "two__"},
            },
        .metas = {vector_one<meta>{
            meta{
                .ints =
                    {
                        1,
                        2,
                        3,
                        4,
                        5,
                    },
            },
            meta{
                .ints = ints1,
            },
        }},
        .single_meta =
            meta{
                .ints = {66, 50, 55},
            },
    };

    const auto json_str = immer::persist::cereal_save_with_pools(test1);
    // REQUIRE(json_str == "");

    {
        auto full_load =
            immer::persist::cereal_load_with_pools<test_data>(json_str);
        REQUIRE(full_load == test1);
        // REQUIRE(immer::persist::cereal_save_with_pools(full_load).first ==
        // "");
    }
}

TEST_CASE("Save with a special pool, special type is enclosed")
{
    const auto map = immer::map<int, std::string>{
        {1, "_one_"},
        {2, "two__"},
    };
    const auto ints1 = test::gen(test::example_vector{}, 3);
    const auto ints5 = vector_one<int>{
        1,
        2,
        3,
        4,
        5,
    };
    const auto metas = vector_one<meta>{
        meta{
            .ints = ints5,
        },
        meta{
            .ints = ints1,
        },
    };
    const auto test1 = test_data{
        .ints      = ints1,
        .strings   = {"one", "two"},
        .flex_ints = flex_vector_one<int>{ints1},
        .map       = map,
        .metas     = metas,
    };
    const auto test2 = test_data{
        .ints      = ints1,
        .strings   = {"three"},
        .flex_ints = flex_vector_one<int>{ints1},
        .map       = map.set(3, "__three"),
        .metas =
            {
                meta{
                    .ints = ints5,
                },
            },
    };

    // At the beginning, the vector is shared, it's the same data.
    REQUIRE(test1.ints.container.identity() == test2.ints.container.identity());
    REQUIRE(test1.flex_ints.container.identity() ==
            test2.flex_ints.container.identity());

    const auto json_str = immer::persist::cereal_save_with_pools(
        std::make_pair(test1, test2),
        immer::persist::via_get_pools_types_policy{});

    // REQUIRE(json_str == "");

    {
        auto [loaded1, loaded2] = immer::persist::cereal_load_with_pools<
            std::pair<test_data, test_data>>(json_str);
        REQUIRE(loaded1 == test1);
        REQUIRE(loaded2 == test2);

        REQUIRE(loaded1.metas.container.size() == 2);
        REQUIRE(loaded1.metas.container[0].ints == ints5);
        REQUIRE(loaded1.metas.container[1].ints == ints1);

        // After loading, two vectors are still reused.
        REQUIRE(loaded1.ints.container.identity() ==
                loaded2.ints.container.identity());
        REQUIRE(loaded1.flex_ints.container.identity() ==
                loaded2.flex_ints.container.identity());

        REQUIRE(loaded1.metas.container[0].ints.container.identity() ==
                loaded2.metas.container[0].ints.container.identity());
    }
}

TEST_CASE("Special pool must load and save types that have no pool")
{
    const auto val1  = test_value{123, "value1"};
    const auto val2  = test_value{234, "value2"};
    const auto value = std::make_pair(val1, val2);

    const auto json_pool_str = immer::persist::cereal_save_with_pools(value);
    REQUIRE(json_pool_str == test::to_json(value));

    {
        auto loaded = immer::persist::cereal_load_with_pools<
            std::decay_t<decltype(value)>>(json_pool_str);
        REQUIRE(loaded == value);
    }
}

TEST_CASE("Special pool loads empty test_data")
{
    const auto value = test_data{};

    // const auto json_pool_str_ = immer::persist::cereal_save_with_pools<
    //     test_data,
    //     immer::persist::name_from_map_fn<decltype(get_pools_types(value))>>(
    //     value);
    // REQUIRE(json_pool_str_ == "");

    const auto json_pool_str = R"(
{
  "value0": {
    "ints": 0,
    "strings": 0,
    "flex_ints": 0,
    "map": 0,
    "metas": 0,
    "metas_map": 0,
    "vectors_map": 0,
    "single_meta": {"ints": 0, "metas": 0},
    "box": 0
  },
  "pools": {
    "ints": {
      "B": 5,
      "BL": 1,
      "leaves": [[1, []]],
      "inners": [[0, {"children": [], "relaxed": false}]],
      "vectors": [{"root": 0, "tail": 1}]
    },
    "strings": {
      "B": 5,
      "BL": 1,
      "leaves": [[1, []]],
      "inners": [[0, {"children": [], "relaxed": false}]],
      "vectors": [{"root": 0, "tail": 1}]
    },
    "flex_ints": {
      "B": 5,
      "BL": 1,
      "leaves": [[1, []]],
      "inners": [[0, {"children": [], "relaxed": false}]],
      "vectors": [{"root": 0, "tail": 1}]
    },
    "int_string_map": [
      {
        "values": [],
        "children": [],
        "nodemap": 0,
        "datamap": 0,
        "collisions": false
      }
    ],
    "metas": {
      "B": 5,
      "BL": 1,
      "leaves": [[1, []]],
      "inners": [[0, {"children": [], "relaxed": false}]],
      "vectors": [{"root": 0, "tail": 1}]
    },
    "meta_metas": {
      "B": 5,
      "BL": 1,
      "leaves": [[1, []]],
      "inners": [[0, {"children": [], "relaxed": false}]],
      "vectors": [{"root": 0, "tail": 1}]
    },
    "table_test_value": [],
    "int_meta_map": [
      {
        "values": [],
        "children": [],
        "nodemap": 0,
        "datamap": 0,
        "collisions": false
      }
    ],
    "int_vector_map": [
      {
        "values": [],
        "children": [],
        "nodemap": 0,
        "datamap": 0,
        "collisions": false
      }
    ],
    "string_box": [""]
  }
}
    )";

    {
        auto loaded = immer::persist::cereal_load_with_pools<
            std::decay_t<decltype(value)>>(
            json_pool_str, immer::persist::via_get_pools_names_policy(value));
        REQUIRE(loaded == value);
    }
}

TEST_CASE("Special pool throws cereal::Exception")
{
    const auto value = test_data{};

    // const auto json_pool_str =
    //     immer::persist::cereal_save_with_pools(value);
    // REQUIRE(json_pool_str == "");

    const auto json_pool_str = R"(
{
  "value0": {
    "ints": 99,
    "strings": 0,
    "flex_ints": 0,
    "map": 0,
    "metas": 0,
    "single_meta": {"ints": 0, "metas": 0}
  },
  "pools": {
    "ints": {
      "B": 5,
      "BL": 1,
      "leaves": [[1, []]],
      "inners": [[0, {"children": [], "relaxed": false}]],
      "vectors": [{"root": 0, "tail": 1}]
    },
    "strings": {
      "B": 5,
      "BL": 1,
      "leaves": [[1, []]],
      "inners": [[0, {"children": [], "relaxed": false}]],
      "vectors": [{"root": 0, "tail": 1}]
    },
    "flex_ints": {
      "B": 5,
      "BL": 1,
      "leaves": [[1, []]],
      "inners": [[0, {"children": [], "relaxed": false}]],
      "vectors": [{"root": 0, "tail": 1}]
    },
    "int_string_map": [
      {
        "values": [],
        "children": [],
        "nodemap": 0,
        "datamap": 0,
        "collisions": false
      }
    ],
    "metas": {
      "B": 5,
      "BL": 1,
      "leaves": [[1, []]],
      "inners": [[0, {"children": [], "relaxed": false}]],
      "vectors": [{"root": 0, "tail": 1}]
    },
    "meta_metas": {
      "B": 5,
      "BL": 1,
      "leaves": [[1, []]],
      "inners": [[0, {"children": [], "relaxed": false}]],
      "vectors": [{"root": 0, "tail": 1}]
    },
    "table_test_value": [],
    "int_meta_map": [],
    "int_vector_map": [],
    "string_box": []
  }
}
    )";

    const auto call = [&] {
        return immer::persist::cereal_load_with_pools<test_data>(
            json_pool_str,
            immer::persist::via_get_pools_names_policy(test_data{}));
    };
    REQUIRE_THROWS_MATCHES(
        call(),
        ::cereal::Exception,
        MessageMatches(Catch::Matchers::ContainsSubstring(
                           "Failed to load a container ID 99 from the "
                           "pool of immer::vector<int") &&
                       Catch::Matchers::ContainsSubstring(
                           "Container ID 99 is not found")));
}

namespace {
struct recursive_type
{
    int data;
    per<vector_one<per<immer::box<recursive_type>>>> children;

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(data), CEREAL_NVP(children));
    }

    auto tie() const { return std::tie(data, children); }

    friend bool operator==(const recursive_type& left,
                           const recursive_type& right)
    {
        return left.tie() == right.tie();
    }
};

auto get_pools_types(const recursive_type&)
{
    return hana::tuple_t<immer::box<recursive_type>,
                         vector_one<per<immer::box<recursive_type>>>>;
}

} // namespace

TEST_CASE("Test recursive type")
{
    const auto v1 = recursive_type{
        .data = 123,
    };
    const auto v2 = recursive_type{
        .data = 234,
    };
    const auto v3 = recursive_type{
        .data     = 345,
        .children = {immer::box<recursive_type>{v1},
                     immer::box<recursive_type>{v2}},
    };

    const auto json_str = immer::persist::cereal_save_with_pools(v3);
    // REQUIRE(json_str == "");

    {
        auto full_load =
            immer::persist::cereal_load_with_pools<recursive_type>(json_str);
        REQUIRE(full_load == v3);
        REQUIRE(immer::persist::cereal_save_with_pools(full_load) == json_str);
    }
}

TEST_CASE("Test recursive type, saving the box triggers saving the box of the "
          "same type")
{
    const auto v1 = recursive_type{
        .data = 123,
    };
    const auto v2 = recursive_type{
        .data = 234,
    };
    const auto v3 = recursive_type{
        .data     = 345,
        .children = {immer::box<recursive_type>{v1},
                     immer::box<recursive_type>{v2}},
    };
    // NOTE: v3 is boxed and inside of it, it contains more boxed values.
    const auto v4 = recursive_type{
        .data     = 456,
        .children = {immer::box<recursive_type>{v1},
                     immer::box<recursive_type>{v3}},
    };
    const auto v5 = recursive_type{
        .data = 567,
        .children =
            {
                immer::box<recursive_type>{v4},
            },
    };

    const auto json_str = immer::persist::cereal_save_with_pools(v5);
    // REQUIRE(json_str == "");

    {
        const auto full_load =
            immer::persist::cereal_load_with_pools<recursive_type>(json_str);
        REQUIRE(full_load == v5);
        REQUIRE(immer::persist::cereal_save_with_pools(full_load) == json_str);
    }
}

namespace {
struct rec_map
{
    int data;
    per<immer::map<int, per<immer::box<rec_map>>>> map;

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(data), CEREAL_NVP(map));
    }

    auto tie() const { return std::tie(data, map); }

    friend bool operator==(const rec_map& left, const rec_map& right)
    {
        return left.tie() == right.tie();
    }
};

auto get_pools_types(const rec_map&)
{
    return hana::tuple_t<immer::box<rec_map>,
                         immer::map<int, per<immer::box<rec_map>>>>;
}

struct same_name_policy : immer::persist::via_get_pools_types_policy
{
    template <class T>
    auto get_pool_name(const T& value) const
    {
        return "same_pool_name";
    }
};
} // namespace

TEST_CASE("Test saving a map that contains the same map")
{
    const auto v1    = immer::box<rec_map>{rec_map{
           .data = 123,
    }};
    const auto v2    = immer::box<rec_map>{rec_map{
           .data = 234,
           .map =
               {
                {1, v1},
            },
    }};
    const auto value = rec_map{
        .data = 345,
        .map =
            {
                {2, v2},
            },
    };

    const auto json_str = immer::persist::cereal_save_with_pools(value);
    // REQUIRE(json_str == "");

    {
        const auto full_load =
            immer::persist::cereal_load_with_pools<rec_map>(json_str);
        REQUIRE(full_load == value);
        REQUIRE(immer::persist::cereal_save_with_pools(full_load) == json_str);
    }

    SECTION("Duplicate pool names are detected")
    {
        REQUIRE_THROWS_AS(
            immer::persist::cereal_save_with_pools(value, same_name_policy{}),
            immer::persist::duplicate_name_pool_detected);
    }
}

TEST_CASE("Test non-unique names in the map")
{
    const auto names =
        hana::make_map(hana::make_pair(hana::type_c<immer::box<int>>,
                                       BOOST_HANA_STRING("box_1")),
                       hana::make_pair(hana::type_c<immer::box<std::string>>,
                                       BOOST_HANA_STRING("box_2"))

        );

    using IsUnique =
        decltype(immer::persist::detail::are_type_names_unique(names));
    static_assert(IsUnique::value, "Names are unique");
}
