#pragma once

#include "archive.hpp"
#include "load.hpp"
#include "save.hpp"

#include <optional>

namespace immer_archive {
namespace champ {

template <class Node>
struct node_traits
{
    template <typename T>
    struct impl;

    template <typename T,
              typename Hash,
              typename Equal,
              typename MemoryPolicy,
              immer::detail::hamts::bits_t B>
    struct impl<immer::detail::hamts::node<T, Hash, Equal, MemoryPolicy, B>>
    {
        using equal_t              = Equal;
        using hash_t               = Hash;
        using memory_t             = MemoryPolicy;
        static constexpr auto bits = B;
    };

    using Hash                 = typename impl<Node>::hash_t;
    using Equal                = typename impl<Node>::equal_t;
    using MemoryPolicy         = typename impl<Node>::memory_t;
    static constexpr auto bits = impl<Node>::bits;
};

class hash_validation_failed_exception : public archive_exception
{
public:
    hash_validation_failed_exception()
        : archive_exception{"Hash validation failed, likely different hash "
                            "algos are used for saving and loading"}
    {
    }
};

template <class Container>
class container_loader
{
    using champ_t = std::decay_t<decltype(std::declval<Container>().impl())>;
    using node_t  = typename champ_t::node_t;
    using value_t = typename node_t::value_t;
    using traits  = node_traits<node_t>;

    struct project_value_ptr
    {
        const value_t* operator()(const value_t& v) const noexcept
        {
            return &v;
        }
    };

public:
    explicit container_loader(container_archive_load<Container> archive)
        : archive_{std::move(archive)}
        , nodes_{archive_.nodes}
    {
    }

    Container load(node_id root_id)
    {
        if (root_id.value >= archive_.nodes.size()) {
            throw invalid_node_id{root_id};
        }

        auto [root, values]    = nodes_.load_inner(root_id);
        const auto items_count = [&values = values] {
            auto count = std::size_t{};
            for (const auto& items : values) {
                count += items.size();
            }
            return count;
        }();

        auto impl = champ_t{std::move(root).release(), items_count};

        // Validate the loaded champ by ensuring that all elements can be
        // found. This verifies the hash function is the same as used while
        // saving it.
        for (const auto& items : values) {
            for (const auto& item : items) {
                const auto* p = impl.template get<
                    project_value_ptr,
                    immer::detail::constantly<const value_t*, nullptr>>(item);
                if (!p) {
                    throw hash_validation_failed_exception{};
                }
                if (!(*p == item)) {
                    throw hash_validation_failed_exception{};
                }
            }
        }

        // XXX This ctor is not public in immer.
        return impl;
    }

private:
    const container_archive_load<Container> archive_;
    nodes_loader<typename node_t::value_t,
                 typename traits::Hash,
                 typename traits::Equal,
                 typename traits::MemoryPolicy,
                 traits::bits>
        nodes_;
};

template <class Container>
std::pair<container_archive_save<Container>, node_id>
save_to_archive(Container container, container_archive_save<Container> archive)
{
    const auto& impl = container.impl();
    auto root_id     = node_id{};
    std::tie(archive.nodes, root_id) =
        get_node_id(std::move(archive.nodes), impl.root);

    if (archive.nodes.inners.count(root_id)) {
        // Already been saved
        return {std::move(archive), root_id};
    }

    archive.nodes = save_nodes(impl, std::move(archive.nodes));
    assert(archive.nodes.inners.count(root_id));

    archive.containers =
        std::move(archive.containers).push_back(std::move(container));

    return {std::move(archive), root_id};
}

} // namespace champ
} // namespace immer_archive
