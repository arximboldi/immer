#include <catch2/catch_test_macros.hpp>

#include <immer/extra/persist/cereal/with_pools.hpp>

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
    // Make it a boost::hana Struct.
    // This allows the persist library to determine what pool types are needed
    // and also to name the pools.
    BOOST_HANA_DEFINE_STRUCT(document,
                             (vector_one, ints),
                             (vector_one, ints2) //
    );

    friend bool operator==(const document&, const document&) = default;

    // Make the struct serializable with cereal as usual, nothing special
    // related to immer-persist.
    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(ints), CEREAL_NVP(ints2));
    }
};

using json_t = nlohmann::json;
// include:intro/end-types

} // namespace

TEST_CASE("Docs save with immer-persist", "[docs]")
{
    // include:intro/start-prepare-value
    const auto v1    = vector_one{1, 2, 3};
    const auto v2    = v1.push_back(4).push_back(5).push_back(6);
    const auto value = document{v1, v2};
    // include:intro/end-prepare-value

    SECTION("Without immer-persist")
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

    SECTION("With immer-persist")
    {
        // Immer-persist uses policies to control certain aspects of
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
        // value, the bigger the benefit from using immer-persist.
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
        {"key": 0, "value": {"children": [2], "relaxed": false}},
        {"key": 3, "value": {"children": [2, 5], "relaxed": false}}
      ],
      "leaves": [
        {"key": 1, "value": [3]},
        {"key": 2, "value": [1, 2]},
        {"key": 4, "value": [5, 6]},
        {"key": 5, "value": [3, 4]}
      ],
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
