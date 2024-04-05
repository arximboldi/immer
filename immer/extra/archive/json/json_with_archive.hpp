#pragma once

#include <immer/extra/archive/json/json_immer.hpp>
#include <immer/extra/archive/traits.hpp>
#include <immer/extra/io.hpp>

#include <boost/hana.hpp>

#include <optional>

/**
 * to_json_with_archive
 */

namespace immer::archive {

namespace detail {

namespace hana = boost::hana;

/**
 * Unimplemented class to generate a compile-time error and show what the type T
 * is.
 */
template <class T>
class error_no_archive_for_the_given_type_check_get_archives_types_function;

template <class T>
class error_duplicate_archive_name_found;

template <class T>
class error_missing_archive_for_type;

/**
 * Archives and functions to serialize types that contain archivable data
 * structures.
 */
template <class Storage, class Names>
struct archives_save
{
    using names_t = Names;

    Storage storage;

    template <class Archive>
    void save(Archive& ar) const
    {
        constexpr auto keys = hana::keys(names_t{});
        hana::for_each(keys, [&](auto key) {
            constexpr auto name = names_t{}[key];
            ar(cereal::make_nvp(name.c_str(), storage[key]));
        });
    }

    template <class T>
    auto& get_save_archive()
    {
        using Contains = decltype(hana::contains(storage, hana::type_c<T>));
        constexpr bool contains = hana::value<Contains>();
        if constexpr (!contains) {
            auto err =
                error_no_archive_for_the_given_type_check_get_archives_types_function<
                    T>{};
        }
        return storage[hana::type_c<T>];
    }

    template <class T>
    const auto& get_save_archive() const
    {
        using Contains = decltype(hana::contains(storage, hana::type_c<T>));
        constexpr bool contains = hana::value<Contains>();
        if constexpr (!contains) {
            auto err =
                error_no_archive_for_the_given_type_check_get_archives_types_function<
                    T>{};
        }
        return storage[hana::type_c<T>];
    }

    friend bool operator==(const archives_save& left,
                           const archives_save& right)
    {
        return left.storage == right.storage;
    }
};

template <class Container>
struct archive_type_load
{
    using archive_t = typename container_traits<Container>::load_archive_t;

    archive_t archive = {};
    std::optional<typename container_traits<Container>::loader_t> loader;

    archive_type_load() = default;

    explicit archive_type_load(archive_t archive_)
        : archive{std::move(archive_)}
    {
    }

    archive_type_load(const archive_type_load& other)
        : archive{other.archive}
    {
    }

    archive_type_load& operator=(const archive_type_load& other)
    {
        archive = other.archive;
        return *this;
    }

    friend bool operator==(const archive_type_load& left,
                           const archive_type_load& right)
    {
        return left.archive == right.archive;
    }
};

/**
 * Transforms a given function into another function that:
 *   - If the given function is a function of one argument, nothing changes.
 *   - Otherwise, passes the given argument as the second argument for the
 * function.
 *
 * In other words, takes a function of maybe two arguments and returns a
 * function of just one argument.
 */
constexpr auto inject_argument = [](auto arg, auto func) {
    return [arg, func](auto&& old) {
        const auto is_valid = hana::is_valid(func)(old);
        if constexpr (hana::value<decltype(is_valid)>()) {
            return func(old);
        } else {
            return func(old, arg);
        }
    };
};

template <class Storage, class Names>
struct archives_load
{
    using names_t = Names;

    Storage storage;

    template <class Container>
    auto& get_loader()
    {
        using Contains =
            decltype(hana::contains(storage, hana::type_c<Container>));
        constexpr bool contains = hana::value<Contains>();
        if constexpr (!contains) {
            auto err = error_missing_archive_for_type<Container>{};
        }

        auto& load = storage[hana::type_c<Container>];
        if (!load.loader) {
            load.loader.emplace(load.archive);
        }
        return *load.loader;
    }

    template <class Archive>
    void load(Archive& ar)
    {
        constexpr auto keys = hana::keys(names_t{});
        hana::for_each(keys, [&](auto key) {
            constexpr auto name = names_t{}[key];
            ar(cereal::make_nvp(name.c_str(), storage[key].archive));
        });
    }

    friend bool operator==(const archives_load& left,
                           const archives_load& right)
    {
        return left.storage == right.storage;
    }

    /**
     * Return a new archives_load after applying the described transformations
     * to each archive type.
     */
    template <class ConversionMap>
    auto transform(const ConversionMap& map) const
    {
        const auto transform_pair = [&map](const auto& pair) {
            // If the conversion map doesn't mention the current type, we leave
            // it as is.
            using Contains = decltype(hana::contains(map, hana::first(pair)));
            constexpr bool contains = hana::value<Contains>();
            if constexpr (contains) {
                // Look up the conversion function by the type from the original
                // archive.
                const auto& func    = map[hana::first(pair)];
                const auto& archive = hana::second(pair).archive;

                // Each archive defines the transform_archive function that
                // transforms its leaves with the given function.
                auto new_archive = transform_archive(archive, func);

                using Container = typename decltype(+hana::first(pair))::type;
                using NewContainer = std::decay_t<
                    decltype(container_traits<Container>::transform(func))>;
                return hana::make_pair(
                    hana::type_c<NewContainer>,
                    archive_type_load<NewContainer>{std::move(new_archive)});
            } else {
                return pair;
            }
        };

        auto new_storage = hana::fold_left(
            storage, hana::make_map(), [&transform_pair](auto map, auto pair) {
                return hana::insert(map, transform_pair(pair));
            });
        using NewStorage = decltype(new_storage);
        return archives_load<NewStorage, Names>{std::move(new_storage)};
    }

    /**
     * ConversionMap is a map where keys are types of the original container
     * (hana::type_c<vector_one<model::snapshot>>) and the values are converting
     * functions that are used to convert the archives.
     *
     * The main feature is that the converting function can also take the second
     * argument, a function get_loader, which can be called with a container
     * type (`get_loader(hana::type_c<vector_one<format::track_id>>)`) and will
     * return a reference to a loader that can be used to load other containers
     * that have already been converted.
     *
     * @see test/extra/archive/test_conversion.cpp
     */
    template <class ConversionMap, class GetIdF>
    auto transform_recursive(const ConversionMap& conversion_map,
                             const GetIdF& get_id) const
    {
        // This lambda is only used to determine types and should never be
        // called.
        constexpr auto fake_convert_container = [](auto new_type,
                                                   const auto& old_container) {
            using NewContainerType = typename decltype(new_type)::type;
            throw std::runtime_error{"This should never be called"};
            return NewContainerType{};
        };

        // Return a pair where second is an optional. Optional is already
        // populated if no transformation is required.
        const auto transform_pair_initial = [fake_convert_container,
                                             &conversion_map](
                                                const auto& pair) {
            using Contains =
                decltype(hana::contains(conversion_map, hana::first(pair)));
            constexpr bool contains = hana::value<Contains>();
            if constexpr (contains) {
                // Look up the conversion function by the type from the original
                // archive.
                const auto& func = inject_argument(
                    fake_convert_container, conversion_map[hana::first(pair)]);

                using Container = typename decltype(+hana::first(pair))::type;
                using NewContainer = std::decay_t<
                    decltype(container_traits<Container>::transform(func))>;
                const auto old_key = hana::first(pair);
                return hana::make_pair(
                    hana::type_c<NewContainer>,
                    hana::make_tuple(
                        old_key,
                        std::optional<archive_type_load<NewContainer>>{}));
            } else {
                // If the conversion map doesn't mention the current type, we
                // leave it as is.
                return hana::make_pair(
                    hana::first(pair),
                    hana::make_tuple(hana::first(pair),
                                     std::make_optional(hana::second(pair))));
            }
        };

        // Each archive is wrapped in optional to know if it's already been
        // converted or not, since the order is unknown and depends on the
        // dependencies between the archives.

        // This temporary storage is a map from type to
        // (old_key, optional<archive_type_load>). Some optionals are already
        // populated, if no transformation is required.
        auto optional_storage = hana::fold_left(
            storage,
            hana::make_map(),
            [transform_pair_initial](auto map, auto pair) {
                return hana::insert(map, transform_pair_initial(pair));
            });
        const auto process =
            [this](auto old_key, auto& optional_archive, const auto& func) {
                if (optional_archive) {
                    // The archive has already been processed, do nothing
                    return;
                }
                const auto& archive = storage[old_key].archive;
                // Each archive defines the transform_archive function that
                // transforms its leaves with the given function.
                optional_archive.emplace(transform_archive(archive, func));
            };

        // The get_loader function accepts a new container type as an argument
        // (after transformation, if any) and returns a reference to a loader
        // that is able to load such container types.
        const auto get_loader = [&optional_storage, &conversion_map, process](
                                    const auto& convert_container,
                                    auto new_container_type) -> auto& {
            auto& s              = optional_storage[new_container_type];
            auto& st             = hana::at_c<1>(s);
            const auto old_key   = hana::at_c<0>(s);
            const auto& old_func = conversion_map[old_key];
            auto func            = inject_argument(convert_container, old_func);
            process(old_key, st, func);

            auto& type_load = *st;
            if (!type_load.loader) {
                type_load.loader.emplace(type_load.archive);
            }
            return *type_load.loader;
        };

        const auto convert_container =
            hana::fix([&get_id, get_loader](
                          auto self, auto new_type, const auto& old_container) {
                const auto id = get_id(old_container);
                auto& loader  = get_loader(self, new_type);
                return loader.load(id);
            });

        // Call `process` for each type
        hana::for_each(hana::keys(optional_storage), [&](auto key) {
            auto& s            = optional_storage[key];
            auto& st           = hana::at_c<1>(s);
            const auto old_key = hana::at_c<0>(s);
            constexpr auto needs_conversion =
                hana::value<decltype(hana::contains(conversion_map,
                                                    old_key))>();
            if constexpr (needs_conversion) {
                const auto& old_func = conversion_map[old_key];
                auto func = inject_argument(convert_container, old_func);
                process(old_key, st, func);
            }
        });

        // By now, all optionals have been emplaced.
        auto new_storage = hana::fold_left(
            optional_storage, hana::make_map(), [](auto map, auto pair) {
                // Extract from std::optional
                return hana::insert(
                    map,
                    hana::make_pair(hana::first(pair),
                                    hana::at_c<1>(hana::second(pair)).value()));
            });
        using NewStorage = decltype(new_storage);
        return archives_load<NewStorage, Names>{std::move(new_storage)};
    }
};

inline auto generate_archives_save(auto type_names)
{
    auto storage =
        hana::fold_left(type_names, hana::make_map(), [](auto map, auto pair) {
            using Type = typename decltype(+hana::first(pair))::type;
            return hana::insert(
                map,
                hana::make_pair(
                    hana::first(pair),
                    typename container_traits<Type>::save_archive_t{}));
        });

    using Storage = decltype(storage);
    using Names   = decltype(type_names);
    return archives_save<Storage, Names>{storage};
}

inline auto are_type_names_unique(auto type_names)
{
    auto names_set =
        hana::fold_left(type_names, hana::make_set(), [](auto set, auto pair) {
            return hana::if_(
                hana::contains(set, hana::second(pair)),
                [](auto pair) {
                    return error_duplicate_archive_name_found<
                        decltype(hana::second(pair))>{};
                },
                [&set](auto pair) {
                    return hana::insert(set, hana::second(pair));
                })(pair);
        });
    return hana::length(type_names) == hana::length(names_set);
}

inline auto generate_archives_load(auto type_names)
{
    auto storage =
        hana::fold_left(type_names, hana::make_map(), [](auto map, auto pair) {
            using Type = typename decltype(+hana::first(pair))::type;
            return hana::insert(
                map,
                hana::make_pair(hana::first(pair), archive_type_load<Type>{}));
        });

    using Storage = decltype(storage);
    using Names   = decltype(type_names);
    return archives_load<Storage, Names>{storage};
}

template <class Storage, class Names>
inline auto to_load_archives(const archives_save<Storage, Names>& save_archive)
{
    auto archives = generate_archives_load(Names{});
    boost::hana::for_each(boost::hana::keys(archives.storage), [&](auto key) {
        archives.storage[key].archive =
            to_load_archive(save_archive.storage[key]);
    });
    return archives;
}

} // namespace detail

template <class T>
auto get_archives_types(const T&)
{
    return boost::hana::make_map();
}

template <class Archives>
constexpr bool is_archive_empty()
{
    using Result =
        decltype(boost::hana::is_empty(boost::hana::keys(Archives{}.storage)));
    return boost::hana::value<Result>();
}

// Recursively serializes the archives but not calling finalize
template <class Archives, class SaveArchiveF>
void save_archives_impl(json_immer_output_archive<Archives>& ar,
                        const SaveArchiveF& save_archive)
{
    using Names    = typename Archives::names_t;
    using IsUnique = decltype(detail::are_type_names_unique(Names{}));
    static_assert(boost::hana::value<IsUnique>(),
                  "Archive names for each type must be unique");

    auto& archives = ar.get_output_archives();

    auto prev = archives;
    while (true) {
        // Keep saving archives until everything is saved.
        archives = save_archive(std::move(archives));
        if (prev == archives) {
            break;
        }
        prev = archives;
    }
}

/**
 * Type T must provide a callable free function get_archives_types(const T&).
 */
template <typename T>
auto to_json_with_archive(const T& serializable)
{
    auto archives =
        detail::generate_archives_save(get_archives_types(serializable));
    using Archives = std::decay_t<decltype(archives)>;

    auto os = std::ostringstream{};

    const auto save_archive = [](auto archives) {
        auto os2 = std::ostringstream{};
        auto ar2 = json_immer_output_archive<Archives>{archives, os2};
        ar2(archives);
        return std::move(ar2).get_output_archives();
    };

    {
        auto ar = immer::archive::json_immer_output_archive<Archives>{os};
        ar(serializable);
        if constexpr (!is_archive_empty<Archives>()) {
            save_archives_impl(ar, save_archive);
            ar.finalize();
        }
        archives = std::move(ar).get_output_archives();
    }
    return std::make_pair(os.str(), std::move(archives));
}

template <typename Archives>
auto load_initial_archives(std::istream& is)
{
    auto archives = Archives{};
    if constexpr (is_archive_empty<Archives>()) {
        return archives;
    }

    auto restore = util::istream_snapshot{is};
    auto ar      = cereal::JSONInputArchive{is};
    ar(CEREAL_NVP(archives));
    return archives;
}

template <typename Archives, class ReloadArchiveF>
auto load_archives(std::istream& is,
                   Archives archives,
                   const ReloadArchiveF& reload_archive)
{
    if constexpr (is_archive_empty<Archives>()) {
        return archives;
    }

    auto prev = archives;
    while (true) {
        // Keep reloading until everything is loaded.
        archives = reload_archive(is, std::move(archives));
        if (prev == archives) {
            break;
        }
        prev = archives;
    }

    return archives;
}

constexpr auto reload_archive = [](std::istream& is, auto archives) {
    using Archives = std::decay_t<decltype(archives)>;
    auto restore   = util::istream_snapshot{is};
    auto ar = json_immer_input_archive<Archives>{std::move(archives), is};
    /**
     * NOTE: Critical to clear the archives before loading into it
     * again. I hit a bug when archives contained a vector and every
     * load would append to it, instead of replacing the contents.
     */
    archives = {};
    ar(CEREAL_NVP(archives));
    return archives;
};

template <typename T>
T from_json_with_archive(std::istream& is)
{
    using Archives = std::decay_t<decltype(detail::generate_archives_load(
        get_archives_types(std::declval<T>())))>;
    auto archives =
        load_archives(is, load_initial_archives<Archives>(is), reload_archive);

    auto ar = immer::archive::json_immer_input_archive<Archives>{
        std::move(archives), is};
    auto r = T{};
    ar(r);
    return r;
}

template <typename T>
T from_json_with_archive(const std::string& input)
{
    auto is = std::istringstream{input};
    return from_json_with_archive<T>(is);
}

template <typename T, typename OldType, typename ConversionsMap>
T from_json_with_archive_with_conversion(std::istream& is,
                                         const ConversionsMap& map)
{
    // Load the archives part for the old type
    using OldArchives = std::decay_t<decltype(detail::generate_archives_load(
        get_archives_types(std::declval<OldType>())))>;
    auto archives_old = load_archives(
        is, load_initial_archives<OldArchives>(is), reload_archive);
    auto archives  = archives_old.transform(map);
    using Archives = decltype(archives);

    auto ar = immer::archive::json_immer_input_archive<Archives>{
        std::move(archives), is};
    auto r = T{};
    ar(r);
    return r;
}

template <typename T, typename OldType, typename ConversionsMap>
T from_json_with_archive_with_conversion(const std::string& input,
                                         const ConversionsMap& map)
{
    auto is = std::istringstream{input};
    return from_json_with_archive_with_conversion<T, OldType>(is, map);
}

/**
 * Given an archives_save and a map of transformations, produce a new type of
 * load archive with those transformations applied
 */
template <class Storage, class Names, class ConversionMap>
inline auto transform_save_archive(
    const detail::archives_save<Storage, Names>& old_archives,
    const ConversionMap& conversion_map)
{
    const auto old_load_archives = to_load_archives(old_archives);
    const auto get_id = [&old_archives](const auto& immer_container) {
        using Container = std::decay_t<decltype(immer_container)>;
        const auto& old_archive =
            old_archives.template get_save_archive<Container>();
        const auto [new_archive, id] =
            save_to_archive(immer_container, old_archive);
        if (!(new_archive == old_archive)) {
            throw std::logic_error{
                "Expecting that the container has already been archived"};
        }
        return id;
    };
    return old_load_archives.transform_recursive(conversion_map, get_id);
}

} // namespace immer::archive
