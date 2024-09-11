#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>

#include <immer/extra/persist/detail/array/pool.hpp>

#include <nlohmann/json.hpp>

#include "utils.hpp"

using namespace test;
using json_t = nlohmann::json;

TEST_CASE("Test saving an array")
{
    using Container = immer::array<std::string>;

    const auto test1 = Container{"hello"};
    const auto test2 = test1.push_back("world");

    auto pool = immer::persist::array::output_pool<Container::value_type,
                                                   Container::memory_policy>{};

    auto id1            = immer::persist::container_id{};
    std::tie(pool, id1) = add_to_pool(test1, pool);

    auto id2            = immer::persist::container_id{};
    std::tie(pool, id2) = add_to_pool(test2, pool);

    SECTION("Save again, same id")
    {
        auto id3            = immer::persist::container_id{};
        std::tie(pool, id3) = add_to_pool(test2, pool);
        REQUIRE(id3 == id2);
    }

    const auto pool_str = to_json(pool);
    // REQUIRE(pool_str == "");

    SECTION("Via JSON")
    {
        const auto pool =
            from_json<immer::persist::array::
                          input_pool<std::string, Container::memory_policy>>(
                pool_str);
        auto loader = immer::persist::array::make_loader_for(Container{}, pool);

        const auto loaded1 = loader.load(id1);
        REQUIRE(loaded1 == test1);

        const auto loaded2 = loader.load(id2);
        REQUIRE(loaded2 == test2);

        REQUIRE(loaded2.identity() == loader.load(id2).identity());
    }

    SECTION("Via pool transformation")
    {
        auto loader = immer::persist::array::make_loader_for(
            Container{}, to_input_pool(pool));

        const auto loaded1 = loader.load(id1);
        REQUIRE(loaded1 == test1);

        const auto loaded2 = loader.load(id2);
        REQUIRE(loaded2 == test2);
    }
}

TEST_CASE("Test saving and mutating an array")
{
    using Container = immer::array<std::string>;

    auto test1 = Container{"hello"};
    auto pool  = immer::persist::array::output_pool<Container::value_type,
                                                   Container::memory_policy>{};

    auto id1            = immer::persist::container_id{};
    std::tie(pool, id1) = immer::persist::array::add_to_pool(test1, pool);

    test1 = std::move(test1).push_back("world");

    auto id2            = immer::persist::container_id{};
    std::tie(pool, id2) = immer::persist::array::add_to_pool(test1, pool);

    REQUIRE(id1 != id2);

    auto loader = immer::persist::array::make_loader_for(Container{},
                                                         to_input_pool(pool));

    const auto loaded1 = loader.load(id1);
    REQUIRE(loaded1 == Container{"hello"});

    const auto loaded2 = loader.load(id2);
    REQUIRE(loaded2 == Container{"hello", "world"});
}

TEST_CASE("Array: converting loader can handle exceptions")
{
    using Container  = immer::array<int>;
    const auto array = Container{123};
    const auto [out_pool, array_id] =
        immer::persist::array::add_to_pool(array, {});
    const auto in_pool = to_input_pool(out_pool);

    using Pool = std::decay_t<decltype(in_pool)>;

    SECTION("Transformation works")
    {
        constexpr auto transform = [](const int& val) {
            return fmt::format("_{}_", val);
        };

        using TransformF = std::decay_t<decltype(transform)>;
        using Loader     = immer::persist::array::
            loader<std::string, immer::default_memory_policy, Pool, TransformF>;
        auto loader       = Loader{in_pool, transform};
        const auto loaded = loader.load(array_id);
        REQUIRE(loaded == immer::array<std::string>{transform(array[0])});
    }

    SECTION("Exception is handled")
    {
        constexpr auto transform = [](const int& val) {
            throw std::runtime_error{"exceptional!"};
            return std::string{};
        };

        using TransformF = std::decay_t<decltype(transform)>;
        using Loader     = immer::persist::array::
            loader<std::string, immer::default_memory_policy, Pool, TransformF>;
        auto loader = Loader{in_pool, transform};
        REQUIRE_THROWS_WITH(loader.load(array_id), "exceptional!");
    }
}

namespace {
template <class Container>
struct direct_container_policy : immer::persist::value0_serialize_t
{
    template <class T>
    auto get_pool_types(const T&) const
    {
        return boost::hana::tuple_t<Container>;
    }

    auto get_pool_name(const Container& container) const { return "pool"; }
};
} // namespace

TEST_CASE("Array: save with pools")
{
    using Container   = immer::array<std::string>;
    const auto policy = direct_container_policy<Container>{};
    const auto value  = Container{"hello", "world"};

    const auto json_str = immer::persist::cereal_save_with_pools(value, policy);
    const auto expected_json = json_t::parse(R"(
{
    "value0": 0,
    "pools": {
        "pool": [
            [
                "hello",
                "world"
            ]
        ]
    }
}
    )");
    REQUIRE(json_t::parse(json_str) == expected_json);

    const auto loaded =
        immer::persist::cereal_load_with_pools<Container>(json_str, policy);
    REQUIRE(loaded == value);
}
