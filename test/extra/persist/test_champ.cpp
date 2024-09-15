#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>

#include <immer/extra/persist/detail/champ/traits.hpp>
#include <immer/extra/persist/xxhash/xxhash.hpp>

#include "utils.hpp"

#include <nlohmann/json.hpp>

using namespace test;
using json_t = nlohmann::json;
using immer::persist::node_id;

namespace {

const auto gen_set = [](auto set, int count) {
    for (int i = 0; i < count; ++i) {
        set = std::move(set).insert(fmt::format("{}", i));
    }
    return set;
};

const auto gen_map = [](auto map, int count) {
    for (int i = 0; i < count; ++i) {
        map = std::move(map).set(i, fmt::format("_{}_", i));
    }
    return map;
};

template <class T>
auto gen_table(T table, std::size_t from, std::size_t to)
{
    for (auto i = std::min(from, to); i < std::max(from, to); ++i) {
        table = std::move(table).insert(test_value{i, fmt::format("_{}_", i)});
    }
    return table;
}

const auto into_set = [](const auto& set) {
    using T     = typename std::decay_t<decltype(set)>::value_type;
    auto result = immer::set<T>{};
    for (const auto& item : set) {
        result = std::move(result).insert(item);
    }
    return result;
};

const auto into_map = [](const auto& map) {
    using K     = typename std::decay_t<decltype(map)>::key_type;
    using V     = typename std::decay_t<decltype(map)>::mapped_type;
    auto result = immer::map<K, V>{};
    for (const auto& item : map) {
        result = std::move(result).insert(item);
    }
    return result;
};

struct broken_hash
{
    std::size_t operator()(const std::string& str) const
    {
        if ("10" < str && str < "19") {
            return 123;
        }
        return immer::persist::xx_hash<std::string>{}(str);
    }

    std::size_t operator()(int map_key) const
    {
        if (10 < map_key && map_key < 19) {
            return 123;
        }
        return std::hash<int>{}(map_key);
    }
};

} // namespace

TEST_CASE("Test saving a set")
{
    using Container = immer::set<std::string, broken_hash>;

    const auto set            = gen_set(Container{}, 200);
    const auto [pool, set_id] = immer::persist::champ::add_to_pool(set, {});
    const auto pool_str       = to_json(pool);
    // REQUIRE(pool_str == "") ;

    SECTION("Load with the correct hash")
    {
        const auto loaded_pool =
            from_json<immer::persist::champ::container_input_pool<Container>>(
                pool_str);

        auto loader = immer::persist::champ::container_loader{loaded_pool};
        const auto loaded = loader.load(set_id);
        REQUIRE(into_set(set).size() == set.size());
        for (const auto& item : set) {
            // This is the only thing that actually breaks if the hash of the
            // loaded set is not the same as the hash function of the serialized
            // set.
            REQUIRE(loaded.count(item));
        }
    }

    SECTION("Load with a different hash")
    {
        using WrongContainer   = immer::set<std::string>;
        const auto loaded_pool = from_json<
            immer::persist::champ::container_input_pool<WrongContainer>>(
            pool_str);

        auto loader = immer::persist::champ::container_loader{loaded_pool};
        REQUIRE_THROWS_AS(
            loader.load(set_id),
            immer::persist::champ::hash_validation_failed_exception);
    }
}

TEST_CASE("Test set with xxHash")
{
    using Container =
        immer::set<std::string, immer::persist::xx_hash<std::string>>;

    const auto set            = gen_set(Container{}, 200);
    const auto [pool, set_id] = immer::persist::champ::add_to_pool(set, {});
    const auto pool_str       = to_json(pool);
    // REQUIRE(pool_str == "");

    SECTION("Load with the correct hash")
    {
        const auto loaded_pool =
            from_json<immer::persist::champ::container_input_pool<Container>>(
                pool_str);

        auto loader = immer::persist::champ::container_loader{loaded_pool};
        const auto loaded = loader.load(set_id);
        REQUIRE(into_set(set).size() == set.size());
        for (const auto& item : set) {
            // This is the only thing that actually breaks if the hash of the
            // loaded set is not the same as the hash function of the serialized
            // set.
            REQUIRE(loaded.count(item));
        }
    }
}

TEST_CASE("Test pool conversion, no json")
{
    using Container = immer::set<std::string, broken_hash>;

    const auto set          = gen_set(Container{}, 200);
    const auto set2         = gen_set(set, 300);
    auto [pool, set_id]     = immer::persist::champ::add_to_pool(set, {});
    auto set2_id            = immer::persist::node_id{};
    std::tie(pool, set2_id) = immer::persist::champ::add_to_pool(set2, pool);

    auto loader = immer::persist::champ::container_loader{to_input_pool(pool)};

    const auto check_set = [&loader](auto id, const auto& expected) {
        const auto loaded = loader.load(id);
        REQUIRE(into_set(loaded) == into_set(expected));
        REQUIRE(into_set(expected).size() == expected.size());
        for (const auto& item : expected) {
            // This is the only thing that actually breaks if the hash of the
            // loaded set is not the same as the hash function of the serialized
            // set.
            REQUIRE(loaded.count(item));
        }
        for (const auto& item : loaded) {
            REQUIRE(expected.count(item));
        }
    };

    check_set(set_id, set);
    check_set(set2_id, set2);
}

TEST_CASE("Test save mutated set")
{
    using Container = immer::set<std::string, broken_hash>;

    auto set            = gen_set(Container{}, 200);
    auto [pool, set_id] = immer::persist::champ::add_to_pool(set, {});

    set                     = std::move(set).insert("435");
    auto set_id2            = immer::persist::node_id{};
    std::tie(pool, set_id2) = immer::persist::champ::add_to_pool(set, pool);

    REQUIRE(set_id != set_id2);
}

TEST_CASE("Test saving a map")
{
    using Container = immer::map<int, std::string, broken_hash>;

    const auto map            = gen_map(Container{}, 200);
    const auto [pool, map_id] = immer::persist::champ::add_to_pool(map, {});
    const auto pool_str       = to_json(pool);
    // REQUIRE(pool_str == "");

    SECTION("Load with the correct hash")
    {
        const auto loaded_pool =
            from_json<immer::persist::champ::container_input_pool<Container>>(
                pool_str);

        auto loader = immer::persist::champ::container_loader{loaded_pool};
        const auto loaded = loader.load(map_id);
        REQUIRE(into_map(map).size() == map.size());
        for (const auto& [key, value] : map) {
            // This is the only thing that actually breaks if the hash of the
            // loaded map is not the same as the hash function of the serialized
            // map.
            REQUIRE(loaded.count(key));
            REQUIRE(loaded[key] == value);
        }
    }

    SECTION("Load with a different hash")
    {
        using WrongContainer   = immer::map<int, std::string>;
        const auto loaded_pool = from_json<
            immer::persist::champ::container_input_pool<WrongContainer>>(
            pool_str);

        auto loader = immer::persist::champ::container_loader{loaded_pool};
        REQUIRE_THROWS_AS(
            loader.load(map_id),
            immer::persist::champ::hash_validation_failed_exception);
    }
}

TEST_CASE("Test map pool conversion, no json")
{
    using Container = immer::map<int, std::string, broken_hash>;

    const auto map          = gen_map(Container{}, 200);
    const auto map2         = gen_map(map, 300);
    auto [pool, map_id]     = immer::persist::champ::add_to_pool(map, {});
    auto map2_id            = immer::persist::node_id{};
    std::tie(pool, map2_id) = immer::persist::champ::add_to_pool(map2, pool);

    auto loader = immer::persist::champ::container_loader{to_input_pool(pool)};

    const auto check_map = [&loader](auto id, const auto& expected) {
        const auto loaded = loader.load(id);
        REQUIRE(into_map(loaded) == into_map(expected));
        REQUIRE(into_map(expected).size() == expected.size());
        for (const auto& [key, value] : expected) {
            // This is the only thing that actually breaks if the hash of the
            // loaded map is not the same as the hash function of the serialized
            // map.
            REQUIRE(loaded.count(key));
            REQUIRE(loaded[key] == value);
        }
        for (const auto& [key, value] : loaded) {
            REQUIRE(expected.count(key));
            REQUIRE(expected[key] == value);
        }
    };

    check_map(map_id, map);
    check_map(map2_id, map2);
}

TEST_CASE("Test save mutated map")
{
    auto map = gen_map(immer::map<int, std::string, broken_hash>{}, 200);
    auto [pool, map_id] = immer::persist::champ::add_to_pool(map, {});

    map                     = std::move(map).set(999, "435");
    auto map_id2            = immer::persist::node_id{};
    std::tie(pool, map_id2) = immer::persist::champ::add_to_pool(map, pool);

    REQUIRE(map_id != map_id2);
}

namespace {
template <class T1, class T2, class Verify>
void test_table_types(Verify&& verify)
{
    const auto t1 = gen_table(T1{}, 0, 100);
    const auto t2 = gen_table(t1, 200, 210);

    auto [pool, t1_id] = immer::persist::champ::add_to_pool(t1, {});

    auto t2_id            = immer::persist::node_id{};
    std::tie(pool, t2_id) = immer::persist::champ::add_to_pool(t2, pool);

    const auto pool_str = to_json(pool);
    // REQUIRE(pool_str == "");

    const auto loaded_pool =
        from_json<immer::persist::champ::container_input_pool<T2>>(pool_str);

    auto loader = immer::persist::champ::container_loader{loaded_pool};

    const auto check = [&loader, &verify](auto id, const auto& expected) {
        const auto loaded = loader.load(id);
        verify(loaded, expected);
    };

    check(t1_id, t1);
    check(t2_id, t2);
}
} // namespace

TEST_CASE("Test saving a table")
{
    using different_table_t =
        immer::table<test_value, immer::table_key_fn, broken_hash>;
    using table_t = immer::table<test_value>;

    const auto verify_is_equal = [](const auto& loaded, const auto& expected) {
        for (const auto& item : expected) {
            INFO(item);
            REQUIRE(loaded.count(item.id));
            REQUIRE(loaded[item.id] == item);
        }
        for (const auto& item : loaded) {
            REQUIRE(expected[item.id] == item);
        }
    };

    // Verify that saving and loading with customized hash works.
    test_table_types<table_t, table_t>(verify_is_equal);
    test_table_types<different_table_t, different_table_t>(verify_is_equal);

    const auto test1 = [&] {
        test_table_types<different_table_t, table_t>(verify_is_equal);
    };
    const auto test2 = [&] {
        test_table_types<table_t, different_table_t>(verify_is_equal);
    };

    REQUIRE_THROWS_AS(test1(),
                      immer::persist::champ::hash_validation_failed_exception);
    REQUIRE_THROWS_AS(test2(),
                      immer::persist::champ::hash_validation_failed_exception);
}

TEST_CASE("Test saving a table, no json")
{
    using table_t = immer::table<test_value>;
    const auto t1 = gen_table(table_t{}, 0, 100);
    const auto t2 = gen_table(t1, 200, 210);

    auto [pool, t1_id] = immer::persist::champ::add_to_pool(t1, {});

    auto t2_id            = immer::persist::node_id{};
    std::tie(pool, t2_id) = immer::persist::champ::add_to_pool(t2, pool);

    const auto pool_str = to_json(pool);
    // REQUIRE(pool_str == "");

    auto loader = immer::persist::champ::container_loader{to_input_pool(pool)};

    const auto check = [&loader](auto id, const auto& expected) {
        const auto loaded = loader.load(id);

        for (const auto& item : expected) {
            REQUIRE(loaded[item.id] == item);
        }
        for (const auto& item : loaded) {
            REQUIRE(expected[item.id] == item);
        }
    };

    check(t1_id, t1);
    check(t2_id, t2);
}

TEST_CASE("Test modifying set nodes")
{
    using Container =
        immer::set<std::string, immer::persist::xx_hash<std::string>>;

    const auto expected_set  = gen_set(Container{}, 30);
    const auto expected_set2 = expected_set.insert("thirty");

    auto [pool, set_id] = immer::persist::champ::add_to_pool(expected_set, {});
    auto set2_id        = immer::persist::node_id{};
    std::tie(pool, set2_id) =
        immer::persist::champ::add_to_pool(expected_set2, std::move(pool));
    const auto pool_str      = to_json(pool);
    const auto expected_data = json_t::parse(pool_str);
    // REQUIRE(pool_str == "");

    auto data = json_t::parse(R"({
  "value0": [
      {
        "values": ["15", "27", "6", "0", "1", "20", "4"],
        "children": [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11],
        "nodemap": 1343560972,
        "datamap": 19407009,
        "collisions": false
      },
      {
        "values": ["25", "24"],
        "children": [],
        "nodemap": 0,
        "datamap": 536875008,
        "collisions": false
      },
      {
        "values": ["14", "16"],
        "children": [],
        "nodemap": 0,
        "datamap": 16777224,
        "collisions": false
      },
      {
        "values": ["23", "26", "9"],
        "children": [],
        "nodemap": 0,
        "datamap": 2147747840,
        "collisions": false
      },
      {
        "values": ["13", "8"],
        "children": [],
        "nodemap": 0,
        "datamap": 16512,
        "collisions": false
      },
      {
        "values": ["28", "22"],
        "children": [],
        "nodemap": 0,
        "datamap": 8388640,
        "collisions": false
      },
      {
        "values": ["3", "19"],
        "children": [],
        "nodemap": 0,
        "datamap": 2147485696,
        "collisions": false
      },
      {
        "values": ["10", "5"],
        "children": [],
        "nodemap": 0,
        "datamap": 33556480,
        "collisions": false
      },
      {
        "values": ["29", "18"],
        "children": [],
        "nodemap": 0,
        "datamap": 16778240,
        "collisions": false
      },
      {
        "values": ["2", "12"],
        "children": [],
        "nodemap": 0,
        "datamap": 9,
        "collisions": false
      },
      {
        "values": ["7", "11"],
        "children": [],
        "nodemap": 0,
        "datamap": 8388616,
        "collisions": false
      },
      {
        "values": ["21", "17"],
        "children": [],
        "nodemap": 0,
        "datamap": 268451840,
        "collisions": false
      },
      {
        "values": ["15", "27", "6", "0", "1", "thirty", "20", "4"],
        "children": [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11],
        "nodemap": 1343560972,
        "datamap": 19407011,
        "collisions": false
      }
    ]
})");
    REQUIRE(data == expected_data);

    const auto load_set = [&data](auto id) {
        const auto loaded_pool =
            from_json<immer::persist::champ::container_input_pool<Container>>(
                data.dump());
        auto loader = immer::persist::champ::container_loader{loaded_pool};
        return loader.load(id);
    };

    SECTION("Loads correctly")
    {
        REQUIRE(load_set(set_id) == expected_set);
        REQUIRE(load_set(set2_id) == expected_set2);
    }
    SECTION("Modify nodemap of node 0")
    {
        auto& nodemap = data["value0"][0]["nodemap"];
        REQUIRE(nodemap == 1343560972);
        nodemap = 1343560971;
        REQUIRE_THROWS_AS(
            load_set(set_id),
            immer::persist::champ::children_count_corrupted_exception);
        // Node 0 doesn't affect the second set
        REQUIRE(load_set(set2_id) == expected_set2);
    }
    SECTION("Modify datamap of node 0")
    {
        auto& datamap = data["value0"][0]["datamap"];
        REQUIRE(datamap == 19407009);
        datamap = 19407008;
        REQUIRE_THROWS_AS(
            load_set(set_id),
            immer::persist::champ::data_count_corrupted_exception);
        // Node 0 doesn't affect the second set
        REQUIRE(load_set(set2_id) == expected_set2);
    }
    SECTION("Modify nodemap of node 1")
    {
        auto& nodemap = data["value0"][1]["nodemap"];
        REQUIRE(nodemap == 0);
        nodemap = 1;
        REQUIRE_THROWS_AS(
            load_set(set_id),
            immer::persist::champ::children_count_corrupted_exception);
        REQUIRE_THROWS_AS(
            load_set(set2_id),
            immer::persist::champ::children_count_corrupted_exception);
    }
    SECTION("Modify datamap of node 1")
    {
        auto& datamap = data["value0"][1]["datamap"];
        REQUIRE(datamap == 536875008);
        datamap = 536875007;
        REQUIRE_THROWS_AS(
            load_set(set_id),
            immer::persist::champ::data_count_corrupted_exception);
        REQUIRE_THROWS_AS(
            load_set(set2_id),
            immer::persist::champ::data_count_corrupted_exception);
    }
    SECTION("Corrupt datamap but keep the same popcount")
    {
        auto& datamap = data["value0"][2]["datamap"];
        REQUIRE(datamap == 16777224);
        datamap = 536875008; // This number also has 2 bits set
        REQUIRE_THROWS_AS(
            load_set(set_id),
            immer::persist::champ::hash_validation_failed_exception);
        REQUIRE_THROWS_AS(
            load_set(set2_id),
            immer::persist::champ::hash_validation_failed_exception);
    }
    SECTION("Corrupt nodemap but keep the same popcount")
    {
        auto& nodemap = data["value0"][0]["nodemap"];
        REQUIRE(nodemap == 1343560972);
        nodemap = 1343560460; // This number has the same number of bits set
        REQUIRE_THROWS_AS(
            load_set(set_id),
            immer::persist::champ::hash_validation_failed_exception);
        // Node 0 doesn't affect the second set
        REQUIRE(load_set(set2_id) == expected_set2);
    }
    SECTION("Missing a child node")
    {
        auto& children = data["value0"][0]["children"];
        children       = {1, 2, 99, 4, 5, 6, 7, 8, 9, 10, 11};
        REQUIRE_THROWS_AS(load_set(set_id), immer::persist::invalid_node_id);
        REQUIRE(load_set(set2_id) == expected_set2);
    }
    SECTION("Same identity")
    {
        // Have to keep the loader alive between loads, otherwise there's no way
        // to share the nodes.
        const auto loaded_pool =
            from_json<immer::persist::champ::container_input_pool<Container>>(
                data.dump());
        auto loader = immer::persist::champ::container_loader{loaded_pool};
        REQUIRE(loader.load(node_id{0}).identity() ==
                loader.load(node_id{0}).identity());
    }
}

TEST_CASE("Test modifying nodes with collisions")
{
    using Container = immer::set<std::string, broken_hash>;

    const auto expected_set  = gen_set(Container{}, 30);
    const auto expected_set2 = expected_set.insert("thirty");

    auto [pool, set_id] = immer::persist::champ::add_to_pool(expected_set, {});
    auto set2_id        = immer::persist::node_id{};
    std::tie(pool, set2_id) =
        immer::persist::champ::add_to_pool(expected_set2, std::move(pool));
    const auto pool_str      = to_json(pool);
    const auto expected_data = json_t::parse(pool_str);

    auto data = json_t::parse(R"({
  "value0": [
    {
      "values": ["8", "27", "6", "29", "2", "0", "1", "7", "20", "4"],
      "children": [1, 2, 3, 4, 5, 6],
      "nodemap": 269550856,
      "datamap": 2898085,
      "collisions": false
    },
    {
      "values": ["25", "24"],
      "children": [],
      "nodemap": 0,
      "datamap": 536875008,
      "collisions": false
    },
    {
      "values": ["23", "26", "9"],
      "children": [],
      "nodemap": 0,
      "datamap": 2147747840,
      "collisions": false
    },
    {
      "values": ["28", "22"],
      "children": [],
      "nodemap": 0,
      "datamap": 8388640,
      "collisions": false
    },
    {
      "values": ["3", "19"],
      "children": [],
      "nodemap": 0,
      "datamap": 2147485696,
      "collisions": false
    },
    {
      "values": ["10", "5"],
      "children": [],
      "nodemap": 0,
      "datamap": 33556480,
      "collisions": false
    },
    {
      "values": ["21"],
      "children": [7],
      "nodemap": 134217728,
      "datamap": 268435456,
      "collisions": false
    },
    {
      "values": [],
      "children": [8],
      "nodemap": 16777216,
      "datamap": 0,
      "collisions": false
    },
    {
      "values": [],
      "children": [9],
      "nodemap": 16777216,
      "datamap": 0,
      "collisions": false
    },
    {
      "values": [],
      "children": [10],
      "nodemap": 16777216,
      "datamap": 0,
      "collisions": false
    },
    {
      "values": [],
      "children": [11],
      "nodemap": 16777216,
      "datamap": 0,
      "collisions": false
    },
    {
      "values": [],
      "children": [12],
      "nodemap": 16777216,
      "datamap": 0,
      "collisions": false
    },
    {
      "values": [],
      "children": [13],
      "nodemap": 16777216,
      "datamap": 0,
      "collisions": false
    },
    {
      "values": [],
      "children": [14],
      "nodemap": 16777216,
      "datamap": 0,
      "collisions": false
    },
    {
      "values": [],
      "children": [15],
      "nodemap": 16777216,
      "datamap": 0,
      "collisions": false
    },
    {
      "values": [],
      "children": [16],
      "nodemap": 16777216,
      "datamap": 0,
      "collisions": false
    },
    {
      "values": [],
      "children": [17],
      "nodemap": 16777216,
      "datamap": 0,
      "collisions": false
    },
    {
      "values": [],
      "children": [18],
      "nodemap": 16777216,
      "datamap": 0,
      "collisions": false
    },
    {
      "values": ["18", "17", "16", "15", "14", "13", "12", "11"],
      "children": [],
      "nodemap": 0,
      "datamap": 0,
      "collisions": true
    },
    {
      "values": ["8", "27", "6", "29", "2", "0", "1", "thirty", "7", "20", "4"],
      "children": [1, 2, 3, 4, 5, 6],
      "nodemap": 269550856,
      "datamap": 2898087,
      "collisions": false
    }
  ]
})");
    INFO(pool_str);
    REQUIRE(data == expected_data);

    const auto load_set = [&data](auto id) {
        const auto loaded_pool =
            from_json<immer::persist::champ::container_input_pool<Container>>(
                data.dump());
        auto loader = immer::persist::champ::container_loader{loaded_pool};
        return loader.load(id);
    };

    SECTION("Loads correctly")
    {
        REQUIRE(load_set(set_id) == expected_set);
        REQUIRE(load_set(set2_id) == expected_set2);
    }
    SECTION("Order of collisions doesn't matter")
    {
        auto& node = data["value0"][18];
        REQUIRE(
            node["values"] ==
            json_t::array({"18", "17", "16", "15", "14", "13", "12", "11"}));
        node["values"] = {"15", "16", "17", "18", "14", "13", "12", "11"};
        REQUIRE(load_set(set_id) == expected_set);
        REQUIRE(load_set(set2_id) == expected_set2);

        // Remove "18"
        node["values"] = {"15", "16", "17", "14", "13", "12", "11"};
        REQUIRE(load_set(set_id) == expected_set.erase("18"));
    }
}

namespace {
struct old_hash
{
    std::size_t operator()(const old_type& val) const
    {
        return std::hash<std::string>{}(val.id);
    }
};
struct new_hash
{
    std::size_t operator()(const new_type& val) const
    {
        return std::hash<std::string>{}(val.id);
    }
};
} // namespace

TEST_CASE("Test set conversion breaks counts")
{
    const auto old_set =
        immer::set<old_type, old_hash>{old_type{.id = "", .data = 91},
                                       old_type{.id = "2", .data = 92},
                                       old_type{.id = "3", .data = 93}};
    REQUIRE(old_set.size() == 3);
    const auto [old_out_pool, set_id] =
        immer::persist::champ::add_to_pool(old_set, {});
    const auto old_in_pool = to_input_pool(old_out_pool);

    const auto transform = [](const old_type& val) {
        // The id is messed up which would lead to the new set
        // containing fewer elements (just one, actually)
        return new_type{
            .id   = "",
            .data = val.data,
        };
    };

    using old_pool_t  = std::decay_t<decltype(old_in_pool)>;
    using new_set_t   = immer::set<new_type, new_hash>;
    using transform_t = std::decay_t<decltype(transform)>;

    auto loader = immer::persist::champ::
        container_loader<new_set_t, old_pool_t, transform_t>{old_in_pool,
                                                             transform};

    REQUIRE_THROWS_AS(loader.load(set_id),
                      immer::persist::champ::hash_validation_failed_exception);
}

namespace {
struct key1
{
    std::string data;

    friend bool operator==(const key1& left, const key1& right)
    {
        return left.data == right.data;
    }

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(data));
    }
};
struct key2
{
    std::string data;

    friend bool operator==(const key2& left, const key2& right)
    {
        return left.data == right.data;
    }

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(data));
    }
};
struct key_hash
{
    std::size_t operator()(const key1& value) const
    {
        if (value.data.size() == 3) {
            return 123;
        }
        return std::hash<std::string>{}(value.data);
    }
    std::size_t operator()(const key2& value) const
    {
        if (value.data.size() == 3) {
            return 123;
        }
        return std::hash<std::string>{}(value.data);
    }
};
} // namespace

TEST_CASE("Champ: converting loader can handle exceptions")
{
    using Container = immer::set<key1, key_hash>;
    const auto set  = [] {
        auto result = Container{};
        for (int i = 0; i < 200; ++i) {
            result = std::move(result).insert(key1{fmt::format("_{}_", i)});
        }
        return result;
    }();
    const auto [ar_save, set_id] = immer::persist::champ::add_to_pool(set, {});
    const auto ar_load           = to_input_pool(ar_save);

    using Pool = std::decay_t<decltype(ar_load)>;

    SECTION("Transformation works")
    {
        constexpr auto transform = [](const key1& val) {
            return key2{val.data};
        };
        const auto transform_set = [transform](const auto& set) {
            auto result = immer::set<key2, key_hash>{};
            for (const auto& item : set) {
                result = std::move(result).insert(transform(item));
            }
            return result;
        };

        using TransformF = std::decay_t<decltype(transform)>;
        using Loader     = immer::persist::champ::
            container_loader<immer::set<key2, key_hash>, Pool, TransformF>;
        auto loader       = Loader{ar_load, transform};
        const auto loaded = loader.load(set_id);
        REQUIRE(loaded == transform_set(set));
    }

    SECTION(
        "Exception is handled when it's thrown while converting a normal node")
    {
        constexpr auto transform = [](const key1& val) {
            if (val.data == "_111_") {
                throw std::runtime_error{"it's 111"};
            }
            return key2{val.data};
        };

        using TransformF = std::decay_t<decltype(transform)>;
        using Loader     = immer::persist::champ::
            container_loader<immer::set<key2, key_hash>, Pool, TransformF>;
        auto loader = Loader{ar_load, transform};
        REQUIRE_THROWS_WITH(loader.load(set_id), "it's 111");
    }

    SECTION("Exception is handled when it's thrown while converting a "
            "collision node")
    {
        constexpr auto transform = [](const key1& val) {
            if (val.data == "_3_") {
                throw std::runtime_error{"it's 3"};
            }
            return key2{val.data};
        };

        using TransformF = std::decay_t<decltype(transform)>;
        using Loader     = immer::persist::champ::
            container_loader<immer::set<key2, key_hash>, Pool, TransformF>;
        auto loader = Loader{ar_load, transform};
        REQUIRE_THROWS_WITH(loader.load(set_id), "it's 3");
    }
}
