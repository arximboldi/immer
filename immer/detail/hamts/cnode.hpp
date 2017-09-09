//
// immer - immutable data structures for C++
// Copyright (C) 2016, 2017 Juan Pedro Bolivar Puente
//
// This file is part of immer.
//
// immer is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// immer is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with immer.  If not, see <http://www.gnu.org/licenses/>.
//

#pragma once

#include <immer/detail/combine_standard_layout.hpp>
#include <immer/detail/util.hpp>
#include <immer/detail/hamts/bits.hpp>

#include <cassert>

#ifdef NDEBUG
#define IMMER_HAMTS_TAGGED_NODE 0
#else
#define IMMER_HAMTS_TAGGED_NODE 1
#endif

namespace immer {
namespace detail {
namespace hamts {

template <typename T,
          typename Hash,
          typename Equal,
          typename MemoryPolicy,
          bits_t B>
struct cnode
{
    using node_t      = cnode;

    using memory      = MemoryPolicy;
    using heap_policy = typename memory::heap;
    using heap        = typename heap_policy::type;
    using transience  = typename memory::transience_t;
    using refs_t      = typename memory::refcount;
    using ownee_t     = typename transience::ownee;
    using edit_t      = typename transience::edit;
    using value_t     = T;

    enum class kind_t
    {
        collision,
        inner
    };

    struct collision_t
    {
        count_t count;
        aligned_storage_for<T> buffer;
    };

    struct values_data_t
    {
        aligned_storage_for<T> buffer;
    };

    using values_t = combine_standard_layout_t<
        values_data_t, refs_t>;

    struct inner_t
    {
        bitmap_t  nodemap;
        bitmap_t  datamap;
        values_t* values;
        aligned_storage_for<node_t*> buffer;
    };

    union data_t
    {
        inner_t inner;
        collision_t collision;
    };

    struct impl_data_t
    {
#if IMMER_HAMTS_TAGGED_NODE
        kind_t kind;
#endif
        data_t data;
    };

    using impl_t = combine_standard_layout_t<
        impl_data_t, refs_t>;

    impl_t impl;

    constexpr static std::size_t sizeof_values_n(count_t count)
    {
        return immer_offsetof(values_t, d.buffer)
            + sizeof(values_data_t::buffer) * count;
    }

    constexpr static std::size_t sizeof_collision_n(count_t count)
    {
        return immer_offsetof(impl_t, d.data.collision.buffer)
            + sizeof(values_data_t::buffer) * count;
    }

    constexpr static std::size_t sizeof_inner_n(count_t count)
    {
        return immer_offsetof(impl_t, d.data.inner.buffer)
            + sizeof(inner_t::buffer) * count;
    }

#if IMMER_HAMTS_TAGGED_NODE
    kind_t kind() const
    {
        return impl.d.kind;
    }
#endif

    auto values()
    {
        return (T*) &impl.d.data.inner.values->d.buffer;
    }

    auto children()
    {
        return (node_t**) &impl.d.data.inner.buffer;
    }

    auto datamap() const
    {
        return impl.d.data.inner.datamap;
    }

    auto nodemap() const
    {
        return impl.d.data.inner.nodemap;
    }

    auto collision_count() const
    {
        return impl.d.data.collision.count;
    }

    T* collisions()
    {
        return (T*)&impl.d.data.collision.buffer;
    }

    static refs_t& refs(const values_t* x) { return auto_const_cast(get<refs_t>(*x)); }
    static const ownee_t& ownee(const values_t* x) { return get<ownee_t>(*x); }
    static ownee_t& ownee(values_t* x) { return get<ownee_t>(*x); }

    static refs_t& refs(const node_t* x) { return auto_const_cast(get<refs_t>(x->impl)); }
    static const ownee_t& ownee(const node_t* x) { return get<ownee_t>(x->impl); }
    static ownee_t& ownee(node_t* x) { return get<ownee_t>(x->impl); }

    static node_t* make_inner_n(count_t n)
    {
        assert(n <= branches<B>);
        auto m = heap::allocate(sizeof_inner_n(n));
        auto p = new (m) node_t;
        p->impl.d.data.inner.values = nullptr;
#if IMMER_HAMTS_TAGGED_NODE
        p->impl.d.kind = node_t::kind_t::inner;
#endif
        return p;
    }

    node_t* inc()
    {
        refs(this).inc();
        return this;
    }

    const node_t* inc() const
    {
        refs(this).inc();
        return this;
    }

    bool dec() const { return refs(this).dec(); }
    void dec_unsafe() const { refs(this).dec_unsafe(); }

    static void delete_values(values_t* p, count_t n)
    {
        destroy_n(&p->d.buffer, n);
        heap::deallocate(node_t::sizeof_values_n(n), p);
    }

    static void delete_inner(node_t* p)
    {
        assert(p->kind() == kind_t::inner);
        auto vp = p->impl.d.data.inner.values;
        if (node_t::refs(vp).dec()) {
            auto n = popcount(p->datamap());
            p->delete_values(vp, n);
        }
    }

    static void delete_collision(node_t* p)
    {
        assert(p->kind() == kind_t::collision);
        auto n = p->collision_count();
        destroy_n(p->collisions(), n);
        heap::deallocate(node_t::sizeof_collision_n(n), p);
    }
};

} // namespace hamts
} // namespace detail
} // namespace immer
