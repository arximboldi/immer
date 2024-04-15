#pragma once

#include <immer/extra/archive/common/archive.hpp>

#include <immer/map.hpp>
#include <immer/set.hpp>
#include <immer/table.hpp>
#include <immer/vector.hpp>

#include <cereal/cereal.hpp>
#include <immer/extra/archive/cereal/immer_vector.hpp>

#include <boost/endian/conversion.hpp>

namespace immer::archive::champ {

template <class T, immer::detail::hamts::bits_t B>
struct inner_node_save
{
    using bitmap_t = typename immer::detail::hamts::get_bitmap_type<B>::type;

    values_save<T> values;
    immer::vector<node_id> children;
    bitmap_t nodemap;
    bitmap_t datamap;
    bool collisions{false};

    auto tie() const
    {
        return std::tie(values, children, nodemap, datamap, collisions);
    }

    friend bool operator==(const inner_node_save& left,
                           const inner_node_save& right)
    {
        return left.tie() == right.tie();
    }

    template <class Archive>
    void save(Archive& ar) const
    {
        ar(CEREAL_NVP(values),
           CEREAL_NVP(children),
           cereal::make_nvp("nodemap", boost::endian::native_to_big(nodemap)),
           cereal::make_nvp("datamap", boost::endian::native_to_big(datamap)),
           CEREAL_NVP(collisions));
    }
};

template <class T, immer::detail::hamts::bits_t B>
struct inner_node_load
{
    using bitmap_t = typename immer::detail::hamts::get_bitmap_type<B>::type;

    values_load<T> values;
    immer::vector<node_id> children;
    bitmap_t nodemap;
    bitmap_t datamap;
    bool collisions{false};

    auto tie() const
    {
        return std::tie(values, children, nodemap, datamap, collisions);
    }

    friend bool operator==(const inner_node_load& left,
                           const inner_node_load& right)
    {
        return left.tie() == right.tie();
    }

    template <class Archive>
    void load(Archive& ar)
    {
        ar(CEREAL_NVP(values),
           CEREAL_NVP(children),
           CEREAL_NVP(nodemap),
           CEREAL_NVP(datamap),
           CEREAL_NVP(collisions));
        boost::endian::big_to_native_inplace(nodemap);
        boost::endian::big_to_native_inplace(datamap);
    }
};

template <class T, immer::detail::hamts::bits_t B, class F>
auto transform(const inner_node_load<T, B>& node, F&& func)
{
    using U = std::decay_t<decltype(func(std::declval<T>()))>;
    return inner_node_load<U, B>{
        .values     = transform(node.values, func),
        .children   = node.children,
        .nodemap    = node.nodemap,
        .datamap    = node.datamap,
        .collisions = node.collisions,
    };
}

template <class T, immer::detail::hamts::bits_t B>
struct nodes_save
{
    // Saving is simpler with a map
    immer::map<node_id, inner_node_save<T, B>> inners;

    immer::map<const void*, node_id> node_ptr_to_id;

    friend bool operator==(const nodes_save& left, const nodes_save& right)
    {
        return left.inners == right.inners;
    }
};

template <typename T,
          typename Hash,
          typename Equal,
          typename MemoryPolicy,
          immer::detail::hamts::bits_t B>
std::pair<nodes_save<T, B>, node_id> get_node_id(
    nodes_save<T, B> ar,
    const immer::detail::hamts::node<T, Hash, Equal, MemoryPolicy, B>* ptr)
{
    auto* ptr_void = static_cast<const void*>(ptr);
    if (auto* maybe_id = ar.node_ptr_to_id.find(ptr_void)) {
        auto id = *maybe_id;
        return {std::move(ar), id};
    }

    const auto id     = node_id{ar.node_ptr_to_id.size()};
    ar.node_ptr_to_id = std::move(ar.node_ptr_to_id).set(ptr_void, id);
    return {std::move(ar), id};
}

template <class T, immer::detail::hamts::bits_t B>
using nodes_load = immer::vector<inner_node_load<T, B>>;

template <class T, immer::detail::hamts::bits_t B, class F>
auto transform(const nodes_load<T, B>& nodes, F&& func)
{
    using U     = std::decay_t<decltype(func(std::declval<T>()))>;
    auto result = nodes_load<U, B>{};
    for (const auto& item : nodes) {
        result = std::move(result).push_back(transform(item, func));
    }
    return result;
}

template <template <class, immer::detail::hamts::bits_t> class InnerNodeType,
          class T,
          immer::detail::hamts::bits_t B>
immer::vector<InnerNodeType<T, B>>
linearize_map(const immer::map<node_id, inner_node_save<T, B>>& inners)
{
    auto result = immer::vector<InnerNodeType<T, B>>{};
    for (auto index = std::size_t{}; index < inners.size(); ++index) {
        auto* p = inners.find(node_id{index});
        assert(p);
        const auto& inner = *p;
        result            = std::move(result).push_back(InnerNodeType<T, B>{
                       .values     = inner.values,
                       .children   = inner.children,
                       .nodemap    = inner.nodemap,
                       .datamap    = inner.datamap,
                       .collisions = inner.collisions,
        });
    }
    return result;
}

/**
 * Container is a champ-based container.
 */
template <class Container>
struct container_archive_save
{
    using champ_t = std::decay_t<decltype(std::declval<Container>().impl())>;
    using T       = typename champ_t::node_t::value_t;

    nodes_save<T, champ_t::bits> nodes;

    // Saving the archived container, so that no mutations are allowed to
    // happen.
    immer::vector<Container> containers;

    friend bool operator==(const container_archive_save& left,
                           const container_archive_save& right)
    {
        return left.nodes == right.nodes;
    }

    template <class Archive>
    void save(Archive& ar) const
    {
        // To serialize, just save the list of nodes
        auto inners = linearize_map<inner_node_save>(nodes.inners);
        using cereal::save;
        save(ar, inners);
    }
};

template <class Container>
struct container_archive_load
{
    using champ_t = std::decay_t<decltype(std::declval<Container>().impl())>;
    using T       = typename champ_t::node_t::value_t;

    nodes_load<T, champ_t::bits> nodes;

    friend bool operator==(const container_archive_load& left,
                           const container_archive_load& right)
    {
        return left.nodes == right.nodes;
    }

    template <class Archive>
    void load(Archive& ar)
    {
        using cereal::load;
        load(ar, nodes);
    }
};

template <class Container>
container_archive_load<Container>
to_load_archive(const container_archive_save<Container>& archive)
{
    return {
        .nodes = linearize_map<inner_node_load>(archive.nodes.inners),
    };
}

} // namespace immer::archive::champ
