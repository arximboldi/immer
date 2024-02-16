#pragma once

#include <immer/experimental/immer-archive/alias.hpp>
#include <immer/experimental/immer-archive/cereal/immer_map.hpp>
#include <immer/experimental/immer-archive/cereal/immer_vector.hpp>
#include <immer/experimental/immer-archive/common/archive.hpp>

#include <immer/array.hpp>
#include <immer/flex_vector.hpp>
#include <immer/map.hpp>
#include <immer/vector.hpp>

#include <cereal/cereal.hpp>

namespace immer_archive::rbts {

struct inner_node
{
    immer::vector<node_id> children;
    bool relaxed = {};

    auto tie() const { return std::tie(children, relaxed); }

    friend bool operator==(const inner_node& left, const inner_node& right)
    {
        return left.tie() == right.tie();
    }

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(children), CEREAL_NVP(relaxed));
    }
};

struct rbts_info
{
    node_id root;
    node_id tail;

    auto tie() const { return std::tie(root, tail); }

    friend bool operator==(const rbts_info& left, const rbts_info& right)
    {
        return left.tie() == right.tie();
    }

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(root), CEREAL_NVP(tail));
    }
};

template <typename T,
          typename MemoryPolicy,
          immer::detail::rbts::bits_t B,
          immer::detail::rbts::bits_t BL>
struct archive_save
{
    immer::map<node_id, values_save<T>> leaves;
    immer::map<node_id, inner_node> inners;
    immer::vector<rbts_info> vectors;

    immer::map<rbts_info, container_id> rbts_to_id;
    immer::map<const void*, node_id> node_ptr_to_id;

    // Saving the archived vectors, so that no mutations are allowed to happen.
    immer::vector<immer::vector<T, MemoryPolicy, B, BL>> saved_vectors;
    immer::vector<immer::flex_vector<T, MemoryPolicy, B, BL>>
        saved_flex_vectors;

    template <class Archive>
    void save(Archive& ar) const
    {
        ar(CEREAL_NVP(leaves), CEREAL_NVP(inners), CEREAL_NVP(vectors));
    }
};

template <typename T,
          typename MemoryPolicy,
          immer::detail::rbts::bits_t B,
          immer::detail::rbts::bits_t BL>
inline auto make_save_archive_for(const immer::vector<T, MemoryPolicy, B, BL>&)
{
    return archive_save<T, MemoryPolicy, B, BL>{};
}

template <typename T,
          typename MemoryPolicy,
          immer::detail::rbts::bits_t B,
          immer::detail::rbts::bits_t BL>
inline auto
make_save_archive_for(const immer::flex_vector<T, MemoryPolicy, B, BL>&)
{
    return archive_save<T, MemoryPolicy, B, BL>{};
}

template <typename T>
struct archive_load
{
    immer::map<node_id, values_load<T>> leaves;
    immer::map<node_id, inner_node> inners;
    immer::vector<rbts_info> vectors;

    auto tie() const { return std::tie(leaves, inners, vectors); }

    friend bool operator==(const archive_load& left, const archive_load& right)
    {
        return left.tie() == right.tie();
    }

    template <class Archive>
    void load(Archive& ar)
    {
        ar(CEREAL_NVP(leaves), CEREAL_NVP(inners), CEREAL_NVP(vectors));
    }
};

// This is needed to be able to use the archive that was not read from JSON
// because .data is set only while reading from JSON.
template <typename T,
          typename MemoryPolicy,
          immer::detail::rbts::bits_t B,
          immer::detail::rbts::bits_t BL>
archive_load<T> fix_leaf_nodes(archive_save<T, MemoryPolicy, B, BL> ar)
{
    auto leaves = immer::map<node_id, values_load<T>>{};
    for (const auto& item : ar.leaves) {
        leaves = std::move(leaves).set(item.first, item.second);
    }

    return {
        .leaves  = std::move(leaves),
        .inners  = std::move(ar.inners),
        .vectors = std::move(ar.vectors),
    };
}

} // namespace immer_archive::rbts

namespace std {

template <>
struct hash<immer_archive::rbts::rbts_info>
{
    auto operator()(const immer_archive::rbts::rbts_info& x) const
    {
        const auto boost_combine = [](std::size_t& seed, std::size_t hash) {
            seed ^= hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        };

        auto seed = std::size_t{};
        boost_combine(seed,
                      hash<immer_archive::node_id::rep_t>{}(x.root.value));
        boost_combine(seed,
                      hash<immer_archive::node_id::rep_t>{}(x.tail.value));
        return seed;
    }
};

} // namespace std
