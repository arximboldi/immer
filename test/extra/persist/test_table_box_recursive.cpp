#include <catch2/catch_test_macros.hpp>

#include <immer/extra/persist/cereal/save.hpp>

#include <test/extra/persist/utils.hpp>

#include <nlohmann/json.hpp>

#define DEFINE_OPERATIONS(name)                                                \
    bool operator==(const name& left, const name& right)                       \
    {                                                                          \
        return test::members(left) == test::members(right);                    \
    }                                                                          \
    template <class Archive>                                                   \
    void serialize(Archive& ar, name& m)                                       \
    {                                                                          \
        test::serialize_members(ar, m);                                        \
    }

namespace test_box_table_recursive {

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
        (immer::table<two_boxed, table_key_fn, immer::persist::xx_hash<key>>,
         twos_table));
};

struct value_two
{
    key key = {};
    value_one one;
};

const key& table_key_fn::operator()(const two_boxed& two) const
{
    return two.two.get().key;
}

} // namespace test_box_table_recursive

namespace test_box_table_recursive {

const key& get_table_key(const two_boxed& two) { return two.two.get().key; }

std::size_t xx_hash_value(const two_boxed& value)
{
    return xx_hash_value(value.two.get().key);
}

two_boxed::two_boxed(value_two val)
    : two{val}
{
}

} // namespace test_box_table_recursive

BOOST_HANA_ADAPT_STRUCT(test_box_table_recursive::value_two, one, key);

namespace test_box_table_recursive {
DEFINE_OPERATIONS(two_boxed);
DEFINE_OPERATIONS(key);
DEFINE_OPERATIONS(value_one);
DEFINE_OPERATIONS(value_two);
} // namespace test_box_table_recursive

using namespace test_box_table_recursive;
namespace hana = boost::hana;
using json_t   = nlohmann::json;

TEST_CASE("Test boxes and tables preserve structural sharing")
{
    const auto two1 = two_boxed{value_two{
        .key = key{"456"},
    }};
    const auto t1 =
        immer::table<two_boxed, table_key_fn, immer::persist::xx_hash<key>>{
            two1};
    const auto two2 = two_boxed{value_two{
        .key = key{"123"},
        .one =
            value_one{
                .twos_table = t1,
            },
    }};
    const auto t2   = t1.insert(two2);
    const auto two3 = two_boxed{value_two{
        .key = key{"789"},
        .one =
            value_one{
                .twos_table = t2,
            },
    }};

    const auto value = value_one{
        .twos_table = t2.insert(two3),
    };

    SECTION("The original box is reused")
    {
        const auto box1 = value.twos_table[key{"456"}].two;

        const auto nested_table =
            value.twos_table[key{"123"}].two.get().one.twos_table;
        const auto box2 = nested_table[key{"456"}].two;
        REQUIRE(box1.impl() == box2.impl());
    }

    const auto json_str = immer::persist::cereal_save_with_pools(
        value, immer::persist::hana_struct_auto_member_name_policy(value));
    // REQUIRE(json_str == "");
    constexpr auto expected_json_str = R"(
{
  "pools": {
    "two": [
      {"key": {"str": "789"}, "one": {"twos_table": 1}},
      {"key": {"str": "456"}, "one": {"twos_table": 2}},
      {"key": {"str": "123"}, "one": {"twos_table": 3}}
    ],
    "twos_table": [
      {
        "children": [],
        "collisions": false,
        "datamap": 1073745924,
        "nodemap": 0,
        "values": [{"two": 0}, {"two": 1}, {"two": 2}]
      },
      {
        "children": [],
        "collisions": false,
        "datamap": 4100,
        "nodemap": 0,
        "values": [{"two": 1}, {"two": 2}]
      },
      {
        "children": [],
        "collisions": false,
        "datamap": 0,
        "nodemap": 0,
        "values": []
      },
      {
        "children": [],
        "collisions": false,
        "datamap": 4096,
        "nodemap": 0,
        "values": [{"two": 1}]
      }
    ]
  },
  "value0": {"twos_table": 0}
})";
    // Saving works, boxes are saved properly
    REQUIRE(json_t::parse(expected_json_str) == json_t::parse(json_str));

    const auto loaded = immer::persist::cereal_load_with_pools<value_one>(
        json_str,
        immer::persist::hana_struct_auto_member_name_policy(value_one{}));
    REQUIRE(loaded == value);

    SECTION("Loaded value currently fails to reuse the box")
    {
        const auto box1 = loaded.twos_table[key{"456"}].two;

        const auto nested_table =
            loaded.twos_table[key{"123"}].two.get().one.twos_table;
        const auto box2 = nested_table[key{"456"}].two;
        REQUIRE(box1.impl() == box2.impl());
    }

    SECTION("Saved loaded value shows the broken sharing")
    {
        const auto loaded_str = immer::persist::cereal_save_with_pools(
            loaded,
            immer::persist::hana_struct_auto_member_name_policy(loaded));
        // REQUIRE(str == "");
        REQUIRE(json_t::parse(loaded_str) == json_t::parse(expected_json_str));
    }
}
