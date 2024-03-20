#pragma once

#include <immer/extra/archive/common/archive.hpp>
#include <immer/extra/archive/errors.hpp>
#include <immer/extra/archive/traits.hpp>

#include <immer/box.hpp>
#include <immer/map.hpp>
#include <immer/vector.hpp>

#include <cereal/cereal.hpp>

namespace immer::archive::box {

template <typename T, typename MemoryPolicy>
struct archive_save
{
    immer::map<container_id, immer::box<T, MemoryPolicy>> boxes;

    immer::map<const void*, container_id> ids;

    friend bool operator==(const archive_save& left, const archive_save& right)
    {
        return left.boxes == right.boxes;
    }

    template <class Archive>
    void save(Archive& ar) const
    {
        ar(cereal::make_size_tag(static_cast<cereal::size_type>(boxes.size())));
        for_each_ordered([&](const auto& box) { ar(box.get()); });
    }

    template <typename F>
    void for_each_ordered(F&& f) const
    {
        for (auto index = std::size_t{}; index < boxes.size(); ++index) {
            auto* p = boxes.find(container_id{index});
            assert(p);
            f(*p);
        }
    }
};

template <typename T, typename MemoryPolicy>
std::pair<archive_save<T, MemoryPolicy>, container_id>
get_box_id(archive_save<T, MemoryPolicy> ar,
           const immer::box<T, MemoryPolicy>& box)
{
    auto* ptr_void = static_cast<const void*>(box.impl());
    if (auto* maybe_id = ar.ids.find(ptr_void)) {
        return {std::move(ar), *maybe_id};
    }

    const auto id = container_id{ar.ids.size()};
    ar.ids        = std::move(ar.ids).set(ptr_void, id);
    return {std::move(ar), id};
}

template <typename T, typename MemoryPolicy>
inline auto make_save_archive_for(const immer::box<T, MemoryPolicy>&)
{
    return archive_save<T, MemoryPolicy>{};
}

template <typename T, typename MemoryPolicy>
struct archive_load
{
    immer::vector<immer::box<T, MemoryPolicy>> boxes;

    friend bool operator==(const archive_load& left, const archive_load& right)
    {
        return left.boxes == right.boxes;
    }

    template <class Archive>
    void load(Archive& ar)
    {
        cereal::size_type size;
        ar(cereal::make_size_tag(size));

        for (auto i = cereal::size_type{}; i < size; ++i) {
            T x;
            ar(x);
            boxes = std::move(boxes).push_back(std::move(x));
        }
    }
};

template <typename T, typename MemoryPolicy>
archive_load<T, MemoryPolicy>
to_load_archive(const archive_save<T, MemoryPolicy>& archive)
{
    auto result = archive_load<T, MemoryPolicy>{};
    archive.for_each_ordered([&](const auto& box) {
        result.boxes = std::move(result.boxes).push_back(box);
    });
    return result;
}

template <typename T, typename MemoryPolicy>
std::pair<archive_save<T, MemoryPolicy>, container_id>
save_to_archive(immer::box<T, MemoryPolicy> box,
                archive_save<T, MemoryPolicy> archive)
{
    auto id               = container_id{};
    std::tie(archive, id) = get_box_id(std::move(archive), box);

    if (archive.boxes.count(id)) {
        // Already been saved
        return {std::move(archive), id};
    }

    archive.boxes = std::move(archive.boxes).set(id, std::move(box));
    return {std::move(archive), id};
}

template <typename T, typename MemoryPolicy>
class loader
{
public:
    explicit loader(archive_load<T, MemoryPolicy> ar)
        : ar_{std::move(ar)}
    {
    }

    immer::box<T, MemoryPolicy> load(container_id id) const
    {
        if (id.value >= ar_.boxes.size()) {
            throw invalid_container_id{id};
        }
        return ar_.boxes[id.value];
    }

private:
    const archive_load<T, MemoryPolicy> ar_;
};

template <typename T, typename MemoryPolicy>
loader<T, MemoryPolicy> make_loader_for(const immer::box<T, MemoryPolicy>&,
                                        archive_load<T, MemoryPolicy> ar)
{
    return loader<T, MemoryPolicy>{std::move(ar)};
}

} // namespace immer::archive::box

namespace immer::archive {

template <typename T, typename MemoryPolicy>
struct container_traits<immer::box<T, MemoryPolicy>>
{
    using save_archive_t = box::archive_save<T, MemoryPolicy>;
    using load_archive_t = box::archive_load<T, MemoryPolicy>;
    using loader_t       = box::loader<T, MemoryPolicy>;
    using container_id   = immer::archive::container_id;
};

} // namespace immer::archive
