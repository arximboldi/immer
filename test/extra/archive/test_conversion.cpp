#include <catch2/catch_test_macros.hpp>

#include <immer/extra/archive/json/json_with_archive_auto.hpp>

#include "utils.hpp"

#define DEFINE_OPERATIONS(name)                                                \
    friend bool operator==(const name& left, const name& right)                \
    {                                                                          \
        return members(left) == members(right);                                \
    }                                                                          \
    template <class Archive>                                                   \
    void serialize(Archive& ar)                                                \
    {                                                                          \
        serialize_members(ar, *this);                                          \
    }

namespace {

using test::members;
using test::serialize_members;
using test::vector_one;
namespace hana = boost::hana;

/**
 * Goal: convert between two independent hierarchies of types: model and format.
 *
 * Problem: We have a vector_one<snapshot>, which makes an archive with leaves
 * of snapshot. Normally, we need to convert model::snapshot <->
 * format::snapshot. But that involves converting between vector_one<track_id>
 * inside, which we can't do directly, otherwise we loose all structural
 * sharing, it must be done somehow via the archive, as well.
 */

namespace model {

struct track_id
{
    BOOST_HANA_DEFINE_STRUCT(track_id, //
                             (int, tid));
    DEFINE_OPERATIONS(track_id);

    track_id(int v)
        : tid{v}
    {
    }

    track_id() = default;
};

struct arrangement
{
    BOOST_HANA_DEFINE_STRUCT(arrangement, //
                             (vector_one<track_id>, tracks));

    DEFINE_OPERATIONS(arrangement);
};

struct snapshot
{
    BOOST_HANA_DEFINE_STRUCT(snapshot, //
                             (arrangement, arr));

    DEFINE_OPERATIONS(snapshot);
};

struct history
{
    BOOST_HANA_DEFINE_STRUCT(history, //
                             (vector_one<snapshot>, snapshots));

    DEFINE_OPERATIONS(history);
};

inline auto make_example_history()
{
    const auto tracks1 = vector_one<track_id>{1, 2, 3, 4, 5};
    const auto tracks2 = tracks1.push_back(6).push_back(7).push_back(8);

    return history{
        .snapshots =
            {
                snapshot{
                    .arr =
                        arrangement{
                            .tracks = tracks1,
                        },
                },
                snapshot{
                    .arr =
                        arrangement{
                            .tracks = tracks2,
                        },
                },
            },
    };
}

} // namespace model

namespace format {

struct track_id
{
    BOOST_HANA_DEFINE_STRUCT(track_id, //
                             (int, tid));
    DEFINE_OPERATIONS(track_id);
};

struct arrangement
{
    BOOST_HANA_DEFINE_STRUCT(arrangement, //
                             (vector_one<track_id>, tracks));

    DEFINE_OPERATIONS(arrangement);
};

struct snapshot
{
    BOOST_HANA_DEFINE_STRUCT(snapshot, //
                             (arrangement, arr));

    DEFINE_OPERATIONS(snapshot);
};

struct history
{
    BOOST_HANA_DEFINE_STRUCT(history, //
                             (vector_one<snapshot>, snapshots));

    DEFINE_OPERATIONS(history);
};

} // namespace format

template <class T>
class Z;

} // namespace

TEST_CASE("Convert between two hierarchies via JSON compatibility")
{
    const auto model_names = immer::archive::get_archives_for_types(
        hana::tuple_t<model::history, model::arrangement>, hana::make_map());
    const auto format_names = immer::archive::get_archives_for_types(
        hana::tuple_t<format::history, format::arrangement>, hana::make_map());
    (void) format_names;

    const auto [json_str, model_archives] =
        immer::archive::to_json_with_auto_archive(model::make_example_history(),
                                                  model_names);

    const auto model_load_archives = [model_names, model_archives] {
        auto archives =
            immer::archive::detail::generate_archives_load(model_names);
        hana::for_each(hana::keys(archives.storage), [&](auto key) {
            archives.storage[key].archive =
                to_load_archive(model_archives.storage[key]);
        });
        return archives;
    }();

    (void) model_load_archives;

    const auto get_id = [model_archives](const auto& immer_container) {
        using Container        = std::decay_t<decltype(immer_container)>;
        const auto old_archive = model_archives.get_save_archive<Container>();
        const auto [new_archive, id] =
            save_to_archive(immer_container, old_archive);
        if (!(new_archive == old_archive)) {
            throw std::logic_error{
                "Expecting that the container has already been archived"};
        }
        return id;
    };

    const auto map = hana::make_map(
        hana::make_pair(
            hana::type_c<vector_one<model::snapshot>>,
            [get_id](model::snapshot old, const auto& get_loader) { //
                SPDLOG_INFO("converting model::snapshot");
                // To convert the archive of vector<snapshot>, I
                // need the archive of vector_one<track_id>
                const auto model_tracks = old.arr.tracks;
                const auto vector_id    = get_id(model_tracks);
                SPDLOG_INFO("vec id = {}", vector_id.value);
                // Load this vector ID from the "new" archive to
                // "convert" this vector
                auto& format_tracks_loader =
                    get_loader(hana::type_c<vector_one<format::track_id>>);
                (void) format_tracks_loader;
                const auto format_tracks = format_tracks_loader.load(vector_id);
                for (auto x : format_tracks) {
                    SPDLOG_INFO("x = {}", x.tid);
                }
                return format::snapshot{};
            }),
        hana::make_pair(hana::type_c<vector_one<model::track_id>>,
                        [](model::track_id old) { //
                            SPDLOG_INFO("converting model::track_id");
                            return format::track_id{old.tid * 100};
                        })

    );
    const auto format_load_archives =
        model_load_archives.transform_recursive(map);
    (void) format_load_archives;

    // Z<decltype(model_load_archives)> qwe;
    // REQUIRE(json_str == "");

    // const auto map

    // const auto format_archives = model_archives.transform(map);

    // {
    //     const auto loaded =
    //         immer::archive::from_json_with_auto_archive<format::history>(
    //             json_str, format_names);
    //     const auto [json_str2, archives] =
    //         immer::archive::to_json_with_auto_archive(loaded, format_names);
    //     REQUIRE(json_str == json_str2);
    // }
}
