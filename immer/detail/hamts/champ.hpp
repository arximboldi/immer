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
struct champ
{
    struct node;
    static constexpr auto bits      = B;

    using node_t      = node;
    using memory      = MemoryPolicy;
    using heap_policy = typename memory::heap;
    using heap        = typename heap_policy::type;
    using transience  = typename memory::transience_t;
    using refs_t      = typename memory::refcount;
    using ownee_t     = typename transience::ownee;
    using edit_t      = typename transience::edit;
    using value_t     = T;

    struct node
    {
        enum class kind_t
        {
            leaf,
            inner
        };

        struct leaf_t
        {
            aligned_storage_for<T> buffer;
        };

        using values_t = combine_standard_layout_t<
            leaf_t, refs_t>;

        struct inner_t
        {
            bitmap_t  map_children;
            bitmap_t  map_values;
            values_t* values;
            aligned_storage_for<node_t*> buffer;
        };

        union data_t
        {
            inner_t inner;
            leaf_t  leaf;
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
                + sizeof(leaf_t::buffer) * count;
        }

        constexpr static std::size_t sizeof_leaf_n(count_t count)
        {
            return immer_offsetof(impl_t, d.data.leaf.buffer)
                + sizeof(leaf_t::buffer) * count;
        }

        constexpr static std::size_t sizeof_inner_n(count_t count)
        {
            return immer_offsetof(impl_t, d.data.inner.buffer)
                + sizeof(inner_t::buffer) * count;
        }

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
    };

    node_t* root;
    size_t  size;

    static const champ empty;

    template <typename K>
    T* get(const K& k) const
    {
        return nullptr;
    }

    champ add(T v) const
    {
        return *this;
    }
};

template <typename T, typename H, typename Eq, typename MP, bits_t B>
const champ<T, H, Eq, MP, B> champ<T, H, Eq, MP, B>::empty = {
    node_t::make_inner_n(0),
    0,
};

} // namespace hamts
} // namespace detail
} // namespace immer
