#pragma once

#include "archive.hpp"
#include "load.hpp"
#include "save.hpp"

#include <optional>

namespace immer::archive {
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
    explicit hash_validation_failed_exception(const std::string& msg)
        : archive_exception{"Hash validation failed, likely different hash "
                            "algos are used for saving and loading, " +
                            msg}
    {
    }
};

/**
 * incompatible_hash_mode:
 * When values are transformed in a way that changes how they are hashed, the
 * structure of the champ can't be preserved. The only solution is to recreate
 * the container from the values that it should contain.
 *
 * The mode can be enabled by returning incompatible_hash_wrapper from the
 * function that handles the target_container_type_request.
 */
template <class Container,
          typename Archive    = container_archive_load<Container>,
          typename TransformF = boost::hana::id_t,
          bool enable_incompatible_hash_mode = false>
class container_loader
{
    using champ_t    = std::decay_t<decltype(std::declval<Container>().impl())>;
    using node_t     = typename champ_t::node_t;
    using value_t    = typename node_t::value_t;
    using traits     = node_traits<node_t>;
    using nodes_load = std::decay_t<decltype(std::declval<Archive>().nodes)>;

    struct project_value_ptr
    {
        const value_t* operator()(const value_t& v) const noexcept
        {
            return std::addressof(v);
        }
    };

public:
    explicit container_loader(Archive archive)
        requires std::is_same_v<TransformF, boost::hana::id_t>
        : archive_{std::move(archive)}
        , nodes_{archive_.nodes}
    {
    }

    explicit container_loader(Archive archive, TransformF transform)
        : archive_{std::move(archive)}
        , nodes_{archive_.nodes, std::move(transform)}
    {
    }

    Container load(node_id root_id)
    {
        if (root_id.value >= archive_.nodes.size()) {
            throw invalid_node_id{root_id};
        }

        auto [root, values] = nodes_.load_inner(root_id);

        if constexpr (enable_incompatible_hash_mode) {
            if (auto* p = loaded_.find(root_id)) {
                return *p;
            }

            auto result = Container{};
            for (const auto& items : values) {
                for (const auto& item : items) {
                    result = std::move(result).insert(item);
                }
            }
            loaded_ = std::move(loaded_).set(root_id, result);
            return result;
        }

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
                    throw hash_validation_failed_exception{
                        "Couldn't find an element"};
                }
                if (!(*p == item)) {
                    throw hash_validation_failed_exception{
                        "Found element is not equal to the one we were looking "
                        "for"};
                }
            }
        }

        // XXX This ctor is not public in immer.
        return impl;
    }

private:
    const Archive archive_;
    nodes_loader<typename node_t::value_t,
                 typename traits::Hash,
                 typename traits::Equal,
                 typename traits::MemoryPolicy,
                 traits::bits,
                 nodes_load,
                 TransformF>
        nodes_;
    immer::map<node_id, Container> loaded_;
};

template <class Container>
container_loader(container_archive_load<Container> archive)
    -> container_loader<Container>;

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
} // namespace immer::archive
