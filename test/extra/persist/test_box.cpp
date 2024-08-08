#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>

#include <immer/extra/persist/detail/box/pool.hpp>

#include "utils.hpp"

using namespace test;

TEST_CASE("Test saving a box")
{
    using Container = immer::box<std::string>;

    const auto test1 = Container{"hello"};
    const auto test2 = Container{"world"};

    auto pool = immer::persist::box::output_pool<Container::value_type,
                                                 Container::memory_policy>{};

    auto id1            = immer::persist::container_id{};
    std::tie(pool, id1) = immer::persist::box::add_to_pool(test1, pool);

    auto id2            = immer::persist::container_id{};
    std::tie(pool, id2) = immer::persist::box::add_to_pool(test2, pool);

    SECTION("Save again, same id")
    {
        auto id3            = immer::persist::container_id{};
        std::tie(pool, id3) = immer::persist::box::add_to_pool(test2, pool);
        REQUIRE(id3 == id2);
    }

    const auto pool_str = to_json(pool);
    // REQUIRE(pool_str == "");

    SECTION("Via JSON")
    {
        const auto pool =
            from_json<immer::persist::box::
                          input_pool<std::string, Container::memory_policy>>(
                pool_str);
        auto loader = immer::persist::box::make_loader_for(Container{}, pool);

        const auto loaded1 = loader.load(id1);
        REQUIRE(loaded1 == test1);

        const auto loaded2 = loader.load(id2);
        REQUIRE(loaded2 == test2);

        REQUIRE(loaded2.impl() == loader.load(id2).impl());
    }

    SECTION("Via pool transformation")
    {
        auto loader = immer::persist::box::make_loader_for(Container{},
                                                           to_input_pool(pool));

        const auto loaded1 = loader.load(id1);
        REQUIRE(loaded1 == test1);

        const auto loaded2 = loader.load(id2);
        REQUIRE(loaded2 == test2);
    }
}

TEST_CASE("Test saving and mutating a box")
{
    using Container = immer::box<std::string>;

    auto test1 = Container{"hello"};
    auto pool  = immer::persist::box::output_pool<Container::value_type,
                                                 Container::memory_policy>{};

    auto id1            = immer::persist::container_id{};
    std::tie(pool, id1) = immer::persist::box::add_to_pool(test1, pool);

    test1 = std::move(test1).update([](auto str) { return str + " world"; });

    auto id2            = immer::persist::container_id{};
    std::tie(pool, id2) = immer::persist::box::add_to_pool(test1, pool);

    REQUIRE(id1 != id2);

    auto loader =
        immer::persist::box::make_loader_for(Container{}, to_input_pool(pool));

    const auto loaded1 = loader.load(id1);
    REQUIRE(loaded1.get() == "hello");

    const auto loaded2 = loader.load(id2);
    REQUIRE(loaded2.get() == "hello world");
}

namespace {
struct fwd_type;
struct test_type
{
    immer::box<fwd_type> data;
};
struct fwd_type
{
    int data = 123;
};
} // namespace

TEST_CASE("Test box with a fwd declared type")
{
    auto val = test_type{};
    REQUIRE(val.data.get().data == 123);
}

TEST_CASE("Box: converting loader can handle exceptions")
{
    const auto box                = immer::box<int>{123};
    const auto [out_pool, box_id] = immer::persist::box::add_to_pool(box, {});
    const auto in_pool            = to_input_pool(out_pool);

    using Pool = std::decay_t<decltype(in_pool)>;

    SECTION("Transformation works")
    {
        constexpr auto transform = [](const int& val) {
            return fmt::format("_{}_", val);
        };

        using TransformF = std::decay_t<decltype(transform)>;
        using Loader     = immer::persist::box::
            loader<std::string, immer::default_memory_policy, Pool, TransformF>;
        auto loader       = Loader{in_pool, transform};
        const auto loaded = loader.load(box_id);
        REQUIRE(loaded.get() == transform(box.get()));
    }

    SECTION("Exception is handled")
    {
        constexpr auto transform = [](const int& val) {
            throw std::runtime_error{"exceptional!"};
            return std::string{};
        };

        using TransformF = std::decay_t<decltype(transform)>;
        using Loader     = immer::persist::box::
            loader<std::string, immer::default_memory_policy, Pool, TransformF>;
        auto loader = Loader{in_pool, transform};
        REQUIRE_THROWS_WITH(loader.load(box_id), "exceptional!");
    }
}
