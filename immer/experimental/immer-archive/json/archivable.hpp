#pragma once

#include <immer/experimental/immer-archive/errors.hpp>
#include <immer/experimental/immer-archive/json/json_immer.hpp>
#include <immer/experimental/immer-archive/json/json_with_archive.hpp>
#include <immer/experimental/immer-archive/traits.hpp>

#include <fmt/format.h>

namespace immer_archive {

template <class Container>
struct archivable
{
    Container container;

    archivable() = default;

    archivable(std::initializer_list<typename Container::value_type> values)
        : container{std::move(values)}
    {
    }

    archivable(Container container_)
        : container{std::move(container_)}
    {
    }

    friend bool operator==(const archivable& left, const archivable& right)
    {
        return left.container == right.container;
    }

    friend auto begin(const archivable& value)
    {
        return value.container.begin();
    }

    friend auto end(const archivable& value) { return value.container.end(); }
};

template <class Storage, class Names, class Container>
auto save_minimal(
    const json_immer_output_archive<detail::archives_save<Storage, Names>>& ar,
    const archivable<Container>& value)
{
    auto& save_archive =
        const_cast<
            json_immer_output_archive<detail::archives_save<Storage, Names>>&>(
            ar)
            .get_output_archives()
            .template get_save_archive<Container>();
    auto [archive, id] =
        save_to_archive(value.container, std::move(save_archive));
    save_archive = std::move(archive);
    return id.value;
}

// This function must exist because cereal does some checks and it's not
// possible to have only load_minimal for a type without having save_minimal.
template <class Storage, class Names, class Container>
auto save_minimal(
    const json_immer_output_archive<detail::archives_load<Storage, Names>>& ar,
    const archivable<Container>& value) ->
    typename container_traits<Container>::container_id::rep_t
{
    throw std::logic_error{"Should never be called"};
}

template <class ImmerArchives, class Container>
void load_minimal(
    const json_immer_input_archive<ImmerArchives>& ar,
    archivable<Container>& value,
    const typename container_traits<Container>::container_id::rep_t& id)
{
    auto& loader = const_cast<json_immer_input_archive<ImmerArchives>&>(ar)
                       .get_input_archives()
                       .template get_loader<Container>();

    // Have to be specific because for vectors container_id is different from
    // node_id, but for hash-based containers, a container is identified just by
    // its root node.
    using container_id_ = typename container_traits<Container>::container_id;

    try {
        value.container = loader.load(container_id_{id});
    } catch (const archive_exception& ex) {
        throw ::cereal::Exception{
            fmt::format("Failed to load a container ID {} from the archive: {}",
                        id,
                        ex.what())};
    }
}

// This function must exist because cereal does some checks and it's not
// possible to have only load_minimal for a type without having save_minimal.
template <class Archive, class Container>
auto save_minimal(const Archive& ar, const archivable<Container>& value) ->
    typename container_traits<Container>::container_id::rep_t
{
    throw std::logic_error{"Should never be called"};
}

template <class Archive, class Container>
void load_minimal(
    const Archive& ar,
    archivable<Container>& value,
    const typename container_traits<Container>::container_id::rep_t& id)
{
    // This one is actually called while loading with not-yet-fully-loaded
    // archive.
}

} // namespace immer_archive
