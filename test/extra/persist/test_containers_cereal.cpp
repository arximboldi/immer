#include <catch2/catch_test_macros.hpp>

#include <immer/extra/cereal/immer_array.hpp>
#include <immer/extra/cereal/immer_box.hpp>
#include <immer/extra/cereal/immer_map.hpp>
#include <immer/extra/cereal/immer_set.hpp>
#include <immer/extra/cereal/immer_table.hpp>
#include <immer/extra/cereal/immer_vector.hpp>
#include <immer/extra/persist/detail/cereal/compact_map.hpp>

#include <cereal/archives/json.hpp>

#include <nlohmann/json.hpp>

namespace {

using json_t = nlohmann::json;

struct with_id
{
    int id           = 99;
    std::string data = "_data_";

    auto tie() const { return std::tie(id, data); }

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(id), CEREAL_NVP(data));
    }

    friend bool operator==(const with_id& left, const with_id& right)
    {
        return left.tie() == right.tie();
    }

    friend std::ostream& operator<<(std::ostream& s, const with_id& value)
    {
        auto ar = cereal::JSONOutputArchive{s};
        ar(value);
        return s;
    }
};

// A struct with non-empty default constructed containers
struct non_empty_default
{
    immer::vector<int> vec           = {1, 2, 3};
    immer::flex_vector<int> flex_vec = {4, 5, 6};
    immer::array<int> arr            = {7, 8, 9};
    immer::map<int, std::string> map = {
        {1, "one"},
        {2, "two"},
    };
    immer::map<int, with_id> with_auto_id = {
        {1,
         with_id{
             .id   = 1,
             .data = "oner",
         }},
        {2,
         with_id{
             .id   = 2,
             .data = "twoer",
         }},
        {3,
         with_id{
             .id   = 3,
             .data = "threer",
         }},
    };
    immer::map<std::string, int> compact_map = {
        {"one", 1},
        {"two", 2},
    };
    immer::box<std::string> box{"default!"};
    immer::set<int> set         = {6, 7, 8, 9};
    immer::table<with_id> table = {
        with_id{
            .id   = 1,
            .data = "oner",
        },
        with_id{
            .id   = 2,
            .data = "twoer",
        },
        with_id{
            .id   = 3,
            .data = "threer",
        },
    };

    auto tie() const
    {
        return std::tie(vec,
                        flex_vec,
                        arr,
                        map,
                        with_auto_id,
                        compact_map,
                        box,
                        set,
                        table);
    }

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(vec),
           CEREAL_NVP(flex_vec),
           CEREAL_NVP(arr),
           CEREAL_NVP(map),
           CEREAL_NVP(with_auto_id),
           cereal::make_nvp(
               "compact_map",
               immer::persist::detail::make_compact_map(compact_map)),
           CEREAL_NVP(box),
           CEREAL_NVP(set),
           CEREAL_NVP(table));
    }

    friend bool operator==(const non_empty_default& left,
                           const non_empty_default& right)
    {
        return left.tie() == right.tie();
    }

    friend std::ostream& operator<<(std::ostream& s,
                                    const non_empty_default& value)
    {
        auto ar = cereal::JSONOutputArchive{s};
        ar(value);
        return s;
    }
};

} // namespace

TEST_CASE("Test loading struct with non-empty containers")
{
    // Create some non-default data
    const auto value = non_empty_default{
        .vec      = {4, 5},
        .flex_vec = {6, 7},
        .arr      = {1, 2},
        .map =
            {
                {3, "three"},
                {4, "four"},
            },
        .with_auto_id =
            {
                {5,
                 with_id{
                     .id   = 5,
                     .data = "fiver",
                 }},
                {6,
                 with_id{
                     .id   = 6,
                     .data = "sixer",
                 }},
            },
        .compact_map =
            {
                {"three", 3},
                {"four", 4},
            },
        .box = "non-default-box",
        .set = {1, 3, 5},
        .table =
            {
                with_id{
                    .id   = 5,
                    .data = "fiver",
                },
                with_id{
                    .id   = 6,
                    .data = "sixer",
                },
            },
    };
    const auto str = [&] {
        auto os = std::ostringstream{};
        {
            auto ar = cereal::JSONOutputArchive{os};
            ar(value);
        }
        return os.str();
    }();

    const auto expected_json = json_t::parse(R"(
{
  "value0": {
    "arr": [1, 2],
    "box": {"value0": "non-default-box"},
    "compact_map": [["three", 3], ["four", 4]],
    "flex_vec": [6, 7],
    "map": [{"key": 3, "value": "three"}, {"key": 4, "value": "four"}],
    "set": [1, 3, 5],
    "table": [{"data": "fiver", "id": 5}, {"data": "sixer", "id": 6}],
    "vec": [4, 5],
    "with_auto_id": [{"data": "fiver", "id": 5}, {"data": "sixer", "id": 6}]
  }
}
    )");
    REQUIRE(json_t::parse(str) == expected_json);

    const auto loaded_value = [&] {
        auto is = std::istringstream{str};
        auto ar = cereal::JSONInputArchive{is};
        auto r  = non_empty_default{};
        ar(r);
        return r;
    }();
    REQUIRE(value == loaded_value);
}
