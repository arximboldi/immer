#pragma once

#include <immer/extra/archive/json/json_immer.hpp>
#include <immer/extra/archive/traits.hpp>

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

    friend bool operator==(const archives_save& left,
                           const archives_save& right)
    {
        return left.storage == right.storage;
    }
};

template <class Container>
struct archive_type_load
{
    typename container_traits<Container>::load_archive_t archive = {};
    std::optional<typename container_traits<Container>::loader_t> loader;

    archive_type_load() = default;

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

template <class Storage, class Names>
struct archives_load
{
    using names_t = Names;

    Storage storage;

    template <class Container>
    auto& get_loader()
    {
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
            return hana::insert(set, hana::second(pair));
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

} // namespace detail

template <class T>
auto get_archives_types(const T&)
{
    return boost::hana::make_map();
}

template <class Archives>
constexpr bool is_archive_empty(const Archives& archives)
{
    using Result =
        decltype(boost::hana::is_empty(boost::hana::keys(archives.storage)));
    return boost::hana::value<Result>();
}

/**
 * Type T must provide a callable free function get_archives_types(const T&).
 */
template <typename T>
auto to_json_with_archive(const T& serializable)
{
    using IsUnique = decltype(detail::are_type_names_unique(
        get_archives_types(serializable)));
    static_assert(boost::hana::value<IsUnique>(),
                  "Archive names for each type must be unique");
    auto archives =
        detail::generate_archives_save(get_archives_types(serializable));
    using Archives = std::decay_t<decltype(archives)>;

    const auto save_archive = [&archives] {
        auto os2 = std::ostringstream{};
        auto ar2 =
            immer::archive::json_immer_output_archive<Archives>{archives, os2};
        ar2(archives);
        archives = ar2.get_output_archives();
    };

    auto os = std::ostringstream{};

    {
        auto ar = immer::archive::json_immer_output_archive<Archives>{os};
        ar(serializable);
        archives = ar.get_output_archives();

        auto prev = archives;
        while (true) {
            // Keep saving archives until everything is saved.
            save_archive();
            if (prev == archives) {
                break;
            }
            prev = archives;
        }

        ar.get_output_archives() = archives;
        if constexpr (!is_archive_empty(archives)) {
            ar.finalize();
        }
    }
    return std::make_pair(os.str(), std::move(archives));
}

template <typename T>
T from_json_with_archive(const std::string& input)
{
    using Archives = std::decay_t<decltype(detail::generate_archives_load(
        get_archives_types(std::declval<T>())))>;
    auto archives  = Archives{};

    if constexpr (!is_archive_empty(archives)) {
        {
            auto is = std::istringstream{input};
            auto ar = cereal::JSONInputArchive{is};
            ar(CEREAL_NVP(archives));
        }

        const auto reload_archive = [&] {
            auto is = std::istringstream{input};
            auto ar = immer::archive::json_immer_input_archive<Archives>{
                archives, is};
            /**
             * NOTE: Critical to clear the archives before loading into it
             * again. I hit a bug when archives contained a vector and every
             * load would append to it, instead of replacing the contents.
             */
            archives = {};
            ar(CEREAL_NVP(archives));
        };

        auto prev = archives;
        while (true) {
            // Keep reloading until everything is loaded.
            reload_archive();
            if (prev == archives) {
                break;
            }
            prev = archives;
        }
    }

    auto is = std::istringstream{input};
    auto ar = immer::archive::json_immer_input_archive<Archives>{
        std::move(archives), is};
    auto r = T{};
    ar(r);
    return r;
}

} // namespace immer::archive
