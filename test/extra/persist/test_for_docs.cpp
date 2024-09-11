#include <catch2/catch_test_macros.hpp>

#include <immer/extra/persist/cereal/save.hpp>
#include <immer/extra/persist/transform.hpp>

#include "utils.hpp"
#include <nlohmann/json.hpp>

namespace {

// include:intro/start-types
// Set the BL constant to 1, so that only 2 elements are stored in leaves.
// This allows to demonstrate structural sharing even in vectors with just a few
// elements.
using vector_one =
    immer::vector<int, immer::default_memory_policy, immer::default_bits, 1>;

struct document
{
    vector_one ints;
    vector_one ints2;

    auto tie() const { return std::tie(ints, ints2); }

    friend bool operator==(const document& left, const document& right)
    {
        return left.tie() == right.tie();
    }

    // Make the struct serializable with cereal as usual, nothing special
    // related to immer::persist.
    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(ints), CEREAL_NVP(ints2));
    }
};

using json_t = nlohmann::json;
// include:intro/end-types

} // namespace

// Make it a boost::hana Struct.
// This allows the persist library to determine what pool types are needed
// and also to name the pools.
// include:intro/start-adapt-document-for-hana
BOOST_HANA_ADAPT_STRUCT(document, ints, ints2);
// include:intro/end-adapt-document-for-hana

TEST_CASE("Docs save with immer::persist", "[docs]")
{
    // include:intro/start-prepare-value
    const auto v1    = vector_one{1, 2, 3};
    const auto v2    = v1.push_back(4).push_back(5).push_back(6);
    const auto value = document{v1, v2};
    // include:intro/end-prepare-value

    SECTION("Without immer::persist")
    {
        const auto expected_json = json_t::parse(R"(
{"value0": {"ints": [1, 2, 3], "ints2": [1, 2, 3, 4, 5, 6]}}
)");
        const auto str           = [&] {
            // include:intro/start-serialize-with-cereal
            auto os = std::ostringstream{};
            {
                auto ar = cereal::JSONOutputArchive{os};
                ar(value);
            }
            return os.str();
            // include:intro/end-serialize-with-cereal
        }();
        REQUIRE(json_t::parse(str) == expected_json);

        const auto loaded_value = [&] {
            auto is = std::istringstream{str};
            auto ar = cereal::JSONInputArchive{is};
            auto r  = document{};
            ar(r);
            return r;
        }();

        REQUIRE(value == loaded_value);
    }

    SECTION("With immer::persist")
    {
        // immer::persist uses policies to control certain aspects of
        // serialization:
        // - types of pools that should be used
        // - names of those pools
        // include:intro/start-serialize-with-persist
        const auto policy =
            immer::persist::hana_struct_auto_member_name_policy(document{});
        const auto str = immer::persist::cereal_save_with_pools(value, policy);
        // include:intro/end-serialize-with-persist

        // The resulting JSON looks much more complicated for this little
        // example but the more structural sharing is used inside the serialized
        // value, the bigger the benefit from using immer::persist.
        //
        // Notable points for the structure of this JSON:
        // - vectors "ints" and "ints2" are serialized as integers, referring to
        // the vectors inside the pools
        // - there is a "pools" object serialized next to the value itself
        // - the "pools" object contains pools per type of the container, in
        // this example only one, for `immer::vector<int>`
        //
        // The vector pool contains:
        // - B and BL constants for the corresponding `immer::vector` type
        // - "inners" and "leaves" maps that store the actual nodes of the
        // vector
        // - "vectors" list that allows to store the root and tail of the vector
        // structure and to refer to the vector with just one integer:
        // `{"ints": 0, "ints2": 1}`: 0 and 1 refer to the indices of this
        // array.

        // include:intro/start-persist-json
        const auto expected_json = json_t::parse(R"(
{
  "value0": {"ints": 0, "ints2": 1},
  "pools": {
    "ints": {
      "B": 5,
      "BL": 1,
      "inners": [
        [0, {"children": [2], "relaxed": false}],
        [3, {"children": [2, 5], "relaxed": false}]
      ],
      "leaves": [[1, [3]], [2, [1, 2]], [4, [5, 6]], [5, [3, 4]]],
      "vectors": [{"root": 0, "tail": 1}, {"root": 3, "tail": 4}]
    }
  }
}
        )");
        // include:intro/end-persist-json
        REQUIRE(json_t::parse(str) == expected_json);

        const auto loaded_value =
            immer::persist::cereal_load_with_pools<document>(str, policy);
        REQUIRE(value == loaded_value);
    }
}

namespace {
// include:start-doc_2-type
using vector_str = immer::
    vector<std::string, immer::default_memory_policy, immer::default_bits, 1>;

struct extra_data
{
    vector_str comments;

    friend bool operator==(const extra_data& left, const extra_data& right)
    {
        return left.comments == right.comments;
    }

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(comments));
    }
};

struct doc_2
{
    vector_one ints;
    vector_one ints2;
    vector_str strings;
    extra_data extra;

    auto tie() const { return std::tie(ints, ints2, strings, extra); }

    friend bool operator==(const doc_2& left, const doc_2& right)
    {
        return left.tie() == right.tie();
    }

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(ints),
           CEREAL_NVP(ints2),
           CEREAL_NVP(strings),
           CEREAL_NVP(extra));
    }
};
// include:end-doc_2-type

// include:start-doc_2_policy
struct doc_2_policy
{
    template <class T>
    auto get_pool_types(const T&) const
    {
        return boost::hana::tuple_t<vector_one, vector_str>;
    }

    template <class Archive>
    void save(Archive& ar, const doc_2& doc2_value) const
    {
        ar(CEREAL_NVP(doc2_value));
    }

    template <class Archive>
    void load(Archive& ar, doc_2& doc2_value) const
    {
        ar(CEREAL_NVP(doc2_value));
    }

    auto get_pool_name(const vector_one&) const { return "vector_of_ints"; }
    auto get_pool_name(const vector_str&) const { return "vector_of_strings"; }
};
// include:end-doc_2_policy
} // namespace

TEST_CASE("Custom policy", "[docs]")
{
    // include:start-doc_2-cereal_save_with_pools
    const auto v1   = vector_one{1, 2, 3};
    const auto v2   = v1.push_back(4).push_back(5).push_back(6);
    const auto str1 = vector_str{"one", "two"};
    const auto str2 =
        str1.push_back("three").push_back("four").push_back("five");
    const auto value = doc_2{v1, v2, str1, extra_data{str2}};

    const auto str =
        immer::persist::cereal_save_with_pools(value, doc_2_policy{});
    // include:end-doc_2-cereal_save_with_pools

    // include:start-doc_2-json
    const auto expected_json = json_t::parse(R"(
{
  "doc2_value": {"ints": 0, "ints2": 1, "strings": 0, "extra": {"comments": 1}},
  "pools": {
    "vector_of_ints": {
      "B": 5,
      "BL": 1,
      "leaves": [[1, [3]], [2, [1, 2]], [4, [5, 6]], [5, [3, 4]]],
      "inners": [
        [0, {"children": [2], "relaxed": false}],
        [3, {"children": [2, 5], "relaxed": false}]
      ],
      "vectors": [{"root": 0, "tail": 1}, {"root": 3, "tail": 4}]
    },
    "vector_of_strings": {
      "B": 5,
      "BL": 1,
      "leaves": [[1, ["one", "two"]], [3, ["five"]], [4, ["three", "four"]]],
      "inners": [
        [0, {"children": [], "relaxed": false}],
        [2, {"children": [1, 4], "relaxed": false}]
      ],
      "vectors": [{"root": 0, "tail": 1}, {"root": 2, "tail": 3}]
    }
  }
}
    )");
    // include:end-doc_2-json
    REQUIRE(json_t::parse(str) == expected_json);

    // include:start-doc_2-load
    const auto loaded_value =
        immer::persist::cereal_load_with_pools<doc_2>(str, doc_2_policy{});
    // include:end-doc_2-load
    REQUIRE(value == loaded_value);
}

// include:start-document_str
namespace {
struct document_str
{
    vector_str str;
    vector_str str2;

    auto tie() const { return std::tie(str, str2); }

    friend bool operator==(const document_str& left, const document_str& right)
    {
        return left.tie() == right.tie();
    }

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(str), CEREAL_NVP(str2));
    }
};
} // namespace
BOOST_HANA_ADAPT_STRUCT(document_str, str, str2);
// include:end-document_str

TEST_CASE("Transformations", "[docs]")
{
    const auto v1    = vector_one{1, 2, 3};
    const auto v2    = v1.push_back(4).push_back(5).push_back(6);
    const auto value = document{v1, v2};

    // include:start-get_output_pools
    const auto pools = immer::persist::get_output_pools(value);
    // include:end-get_output_pools

    namespace hana = boost::hana;

    SECTION("Into same type")
    {
        // include:start-conversion_map
        const auto conversion_map = hana::make_map(hana::make_pair(
            hana::type_c<vector_one>, [](int val) { return val * 10; }));
        // include:end-conversion_map

        // include:start-transformed_pools
        auto transformed_pools =
            immer::persist::transform_output_pool(pools, conversion_map);
        // include:end-transformed_pools

        // include:start-convert-containers
        const auto new_v1 =
            immer::persist::convert_container(pools, transformed_pools, v1);
        const auto expected_new_v1 = vector_one{10, 20, 30};
        REQUIRE(new_v1 == expected_new_v1);

        const auto new_v2 =
            immer::persist::convert_container(pools, transformed_pools, v2);
        const auto expected_new_v2 = vector_one{10, 20, 30, 40, 50, 60};
        REQUIRE(new_v2 == expected_new_v2);

        const auto new_value = document{new_v1, new_v2};
        // include:end-convert-containers

        // include:start-save-new_value
        const auto policy =
            immer::persist::hana_struct_auto_member_name_policy(document{});
        const auto str =
            immer::persist::cereal_save_with_pools(new_value, policy);
        const auto expected_json = json_t::parse(R"(
{
  "pools": {
    "ints": {
      "B": 5,
      "BL": 1,
      "inners": [
        [0, {"children": [2], "relaxed": false}],
        [3, {"children": [2, 5], "relaxed": false}]
      ],
      "leaves": [[1, [30]], [2, [10, 20]], [4, [50, 60]], [5, [30, 40]]],
      "vectors": [{"root": 0, "tail": 1}, {"root": 3, "tail": 4}]
    }
  },
  "value0": {"ints": 0, "ints2": 1}
}
        )");
        REQUIRE(json_t::parse(str) == expected_json);
        // include:end-save-new_value
    }

    SECTION("Into a different type")
    {
        // include:start-conversion_map-string
        const auto conversion_map = hana::make_map(hana::make_pair(
            hana::type_c<vector_one>,
            [](int val) -> std::string { return fmt::format("_{}_", val); }));
        // include:end-conversion_map-string

        // include:start-convert-vectors-of-strings
        auto transformed_pools =
            immer::persist::transform_output_pool(pools, conversion_map);

        const auto new_v1 =
            immer::persist::convert_container(pools, transformed_pools, v1);
        const auto expected_new_v1 = vector_str{"_1_", "_2_", "_3_"};
        REQUIRE(new_v1 == expected_new_v1);

        const auto new_v2 =
            immer::persist::convert_container(pools, transformed_pools, v2);
        const auto expected_new_v2 =
            vector_str{"_1_", "_2_", "_3_", "_4_", "_5_", "_6_"};
        REQUIRE(new_v2 == expected_new_v2);
        // include:end-convert-vectors-of-strings

        // include:start-save-new_value-str
        const auto new_value = document_str{new_v1, new_v2};
        const auto policy =
            immer::persist::hana_struct_auto_member_name_policy(document_str{});
        const auto str =
            immer::persist::cereal_save_with_pools(new_value, policy);
        const auto expected_json = json_t::parse(R"(
{
  "pools": {
    "str": {
      "B": 5,
      "BL": 1,
      "inners": [
        [0, {"children": [2], "relaxed": false}],
        [3, {"children": [2, 5], "relaxed": false}]
      ],
      "leaves": [
        [1, ["_3_"]],
        [2, ["_1_", "_2_"]],
        [4, ["_5_", "_6_"]],
        [5, ["_3_", "_4_"]]
      ],
      "vectors": [{"root": 0, "tail": 1}, {"root": 3, "tail": 4}]
    }
  },
  "value0": {"str": 0, "str2": 1}
}
        )");
        REQUIRE(json_t::parse(str) == expected_json);
        // include:end-save-new_value-str
    }
}

namespace {
// include:start-direct_container_policy
template <class Container>
struct direct_container_policy : immer::persist::value0_serialize_t
{
    template <class T>
    auto get_pool_types(const T&) const
    {
        return boost::hana::tuple_t<Container>;
    }
};
// include:end-direct_container_policy
} // namespace

TEST_CASE("Transform hash-based containers", "[docs]")
{
    // include:start-prepare-int-map
    using int_map_t =
        immer::map<std::string, int, immer::persist::xx_hash<std::string>>;

    const auto value = int_map_t{{"one", 1}, {"two", 2}};
    const auto pools = immer::persist::get_output_pools(
        value, direct_container_policy<int_map_t>{});
    // include:end-prepare-int-map

    // include:start-prepare-conversion_map
    namespace hana     = boost::hana;
    using string_map_t = immer::
        map<std::string, std::string, immer::persist::xx_hash<std::string>>;

    const auto conversion_map = hana::make_map(hana::make_pair(
        hana::type_c<int_map_t>,
        hana::overload(
            [](const std::pair<std::string, int>& item) {
                return std::make_pair(item.first,
                                      fmt::format("_{}_", item.second));
            },
            [](immer::persist::target_container_type_request) {
                return string_map_t{};
            })));
    // include:end-prepare-conversion_map

    // include:start-transform-map
    auto transformed_pools =
        immer::persist::transform_output_pool(pools, conversion_map);
    const auto new_value =
        immer::persist::convert_container(pools, transformed_pools, value);
    const auto expected_new = string_map_t{{"one", "_1_"}, {"two", "_2_"}};
    REQUIRE(new_value == expected_new);
    // include:end-transform-map
}

namespace {
// include:start-old_item
struct old_item
{
    std::string id;
    int data;

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(id), CEREAL_NVP(data));
    }
};
// include:end-old_item

// include:start-new-table-types
struct new_id_t
{
    std::string id;

    friend bool operator==(const new_id_t& left, const new_id_t& right)
    {
        return left.id == right.id;
    }

    friend std::size_t xx_hash_value(const new_id_t& value)
    {
        return immer::persist::xx_hash<std::string>{}(value.id);
    }
};

struct new_item
{
    new_id_t id;
    std::string data;

    auto tie() const { return std::tie(id, data); }

    friend bool operator==(const new_item& left, const new_item& right)
    {
        return left.tie() == right.tie();
    }
};
// include:end-new-table-types
} // namespace

TEST_CASE("Transform table's ID type", "[docs]")
{
    // include:start-prepare-table-value
    using table_t    = immer::table<old_item,
                                 immer::table_key_fn,
                                 immer::persist::xx_hash<std::string>>;
    const auto value = table_t{old_item{"one", 1}, old_item{"two", 2}};
    const auto pools = immer::persist::get_output_pools(
        value, direct_container_policy<table_t>{});
    // include:end-prepare-table-value

    namespace hana = boost::hana;

    // include:start-prepare-new_table_t-type
    using new_table_t = immer::
        table<new_item, immer::table_key_fn, immer::persist::xx_hash<new_id_t>>;
    // include:end-prepare-new_table_t-type

    SECTION("Keeping the same hash")
    {
        // include:start-prepare-new_table_t-conversion_map
        const auto conversion_map = hana::make_map(hana::make_pair(
            hana::type_c<table_t>,
            hana::overload(
                [](const old_item& item) {
                    return new_item{
                        .id   = new_id_t{item.id},
                        .data = fmt::format("_{}_", item.data),
                    };
                },
                [](immer::persist::target_container_type_request) {
                    return new_table_t{};
                })));
        // include:end-prepare-new_table_t-conversion_map

        // include:start-new_table_t-transformation
        auto transformed_pools =
            immer::persist::transform_output_pool(pools, conversion_map);
        const auto new_value =
            immer::persist::convert_container(pools, transformed_pools, value);
        const auto expected_new =
            new_table_t{new_item{{"one"}, "_1_"}, new_item{{"two"}, "_2_"}};
        REQUIRE(new_value == expected_new);
        // include:end-new_table_t-transformation
    }

    SECTION("Hash is changed and broken")
    {
        // include:start-prepare-new_table_t-broken-conversion_map
        const auto conversion_map = hana::make_map(hana::make_pair(
            hana::type_c<table_t>,
            hana::overload(
                [](const old_item& item) {
                    return new_item{
                        // the ID's data is changed and its hash won't be the
                        // same
                        .id   = new_id_t{item.id + "_key"},
                        .data = fmt::format("_{}_", item.data),
                    };
                },
                [](immer::persist::target_container_type_request) {
                    return new_table_t{};
                })));
        // include:end-prepare-new_table_t-broken-conversion_map

        // include:start-new_table_t-broken-transformation
        auto transformed_pools =
            immer::persist::transform_output_pool(pools, conversion_map);
        REQUIRE_THROWS_AS(
            immer::persist::convert_container(pools, transformed_pools, value),
            immer::persist::champ::hash_validation_failed_exception);
        // include:end-new_table_t-broken-transformation
    }

    SECTION("Hash is changed and works")
    {
        // include:start-prepare-new_table_t-new-hash-conversion_map
        const auto conversion_map = hana::make_map(hana::make_pair(
            hana::type_c<table_t>,
            hana::overload(
                [](const old_item& item) {
                    return new_item{
                        // the ID's data is changed and its hash won't be the
                        // same
                        .id   = new_id_t{item.id + "_key"},
                        .data = fmt::format("_{}_", item.data),
                    };
                },
                [](immer::persist::target_container_type_request) {
                    // We know that the hash is changing and requesting to
                    // transform in a less efficient manner
                    return immer::persist::incompatible_hash_wrapper<
                        new_table_t>{};
                })));
        // include:end-prepare-new_table_t-new-hash-conversion_map

        // include:start-new_table_t-new-hash-transformation
        auto transformed_pools =
            immer::persist::transform_output_pool(pools, conversion_map);
        const auto new_value =
            immer::persist::convert_container(pools, transformed_pools, value);
        const auto expected_new = new_table_t{new_item{{"one_key"}, "_1_"},
                                              new_item{{"two_key"}, "_2_"}};
        REQUIRE(new_value == expected_new);
        // include:end-new_table_t-new-hash-transformation

        // include:start-returned-transformed-container-is-the-same
        const auto new_value_2 =
            immer::persist::convert_container(pools, transformed_pools, value);
        REQUIRE(new_value_2.impl().root == new_value.impl().root);
        // include:end-returned-transformed-container-is-the-same
    }
}

namespace {
// include:start-define-nested-types
struct nested_t
{
    vector_one ints;

    friend bool operator==(const nested_t& left, const nested_t& right)
    {
        return left.ints == right.ints;
    }

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(ints));
    }
};

struct with_nested_t
{
    immer::vector<nested_t> nested;

    friend bool operator==(const with_nested_t& left,
                           const with_nested_t& right)
    {
        return left.nested == right.nested;
    }

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(nested));
    }
};
// include:end-define-nested-types

struct new_nested_t
{
    vector_str str;

    friend bool operator==(const new_nested_t& left, const new_nested_t& right)
    {
        return left.str == right.str;
    }

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(str));
    }
};

struct with_new_nested_t
{
    immer::vector<new_nested_t> nested;

    friend bool operator==(const with_new_nested_t& left,
                           const with_new_nested_t& right)
    {
        return left.nested == right.nested;
    }

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(nested));
    }
};
} // namespace

BOOST_HANA_ADAPT_STRUCT(nested_t, ints);
BOOST_HANA_ADAPT_STRUCT(with_nested_t, nested);
BOOST_HANA_ADAPT_STRUCT(new_nested_t, str);
BOOST_HANA_ADAPT_STRUCT(with_new_nested_t, nested);

TEST_CASE("Transform nested containers", "[docs]")
{
    // include:start-prepare-nested-value
    const auto v1    = vector_one{1, 2, 3};
    const auto v2    = v1.push_back(4).push_back(5).push_back(6);
    const auto value = with_nested_t{
        .nested =
            {
                nested_t{.ints = v1},
                nested_t{.ints = v2},
            },
    };

    const auto policy =
        immer::persist::hana_struct_auto_member_name_policy(with_nested_t{});
    const auto str = immer::persist::cereal_save_with_pools(value, policy);
    // include:end-prepare-nested-value

    // include:start-nested-value-json
    const auto expected_json = json_t::parse(R"(
{
  "pools": {
    "ints": {
      "B": 5,
      "BL": 1,
      "inners": [
        [0, {"children": [2], "relaxed": false}],
        [3, {"children": [2, 5], "relaxed": false}]
      ],
      "leaves": [[1, [3]], [2, [1, 2]], [4, [5, 6]], [5, [3, 4]]],
      "vectors": [{"root": 0, "tail": 1}, {"root": 3, "tail": 4}]
    },
    "nested": {
      "B": 5,
      "BL": 3,
      "inners": [[0, {"children": [], "relaxed": false}]],
      "leaves": [[1, [{"ints": 0}, {"ints": 1}]]],
      "vectors": [{"root": 0, "tail": 1}]
    }
  },
  "value0": {"nested": 0}
}
    )");
    // include:end-nested-value-json
    REQUIRE(json_t::parse(str) == expected_json);

    namespace hana = boost::hana;

    // include:start-nested-conversion_map
    const auto conversion_map = hana::make_map(
        hana::make_pair(
            hana::type_c<vector_one>,
            [](int val) -> std::string { return fmt::format("_{}_", val); }),
        hana::make_pair(
            hana::type_c<immer::vector<nested_t>>,
            [](const nested_t& item, const auto& convert_container) {
                return new_nested_t{
                    .str =
                        convert_container(hana::type_c<vector_str>, item.ints),
                };
            }));
    // include:end-nested-conversion_map

    // include:start-apply-nested-transformations
    const auto pools = immer::persist::get_output_pools(value, policy);
    auto transformed_pools =
        immer::persist::transform_output_pool(pools, conversion_map);

    const auto new_value = with_new_nested_t{
        .nested = immer::persist::convert_container(
            pools, transformed_pools, value.nested),
    };
    // include:end-apply-nested-transformations

    // include:start-verify-nested-value
    const auto expected_new = with_new_nested_t{
        .nested =
            {
                new_nested_t{.str = {"_1_", "_2_", "_3_"}},
                new_nested_t{.str = {"_1_", "_2_", "_3_", "_4_", "_5_", "_6_"}},
            },
    };
    REQUIRE(new_value == expected_new);
    // include:end-verify-nested-value

    // include:start-verify-structural-sharing-of-nested
    const auto transformed_str = immer::persist::cereal_save_with_pools(
        new_value,
        immer::persist::hana_struct_auto_member_name_policy(
            with_new_nested_t{}));
    const auto expected_transformed_json = json_t::parse(R"(
{
  "pools": {
    "nested": {
      "B": 5,
      "BL": 3,
      "inners": [[0, {"children": [], "relaxed": false}]],
      "leaves": [[1, [{"str": 0}, {"str": 1}]]],
      "vectors": [{"root": 0, "tail": 1}]
    },
    "str": {
      "B": 5,
      "BL": 1,
      "inners": [
        [0, {"children": [2], "relaxed": false}],
        [3, {"children": [2, 5], "relaxed": false}]
      ],
      "leaves": [
        [1, ["_3_"]],
        [2, ["_1_", "_2_"]],
        [4, ["_5_", "_6_"]],
        [5, ["_3_", "_4_"]]
      ],
      "vectors": [{"root": 0, "tail": 1}, {"root": 3, "tail": 4}]
    }
  },
  "value0": {"nested": 0}
}
    )");
    // include:end-verify-structural-sharing-of-nested
    REQUIRE(json_t::parse(transformed_str) == expected_transformed_json);
}
