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

#include <immer/detail/hamts/cnode.hpp>

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
    static constexpr auto bits = B;

    using node_t = cnode<T, Hash, Equal, MemoryPolicy, B>;

    node_t* root;
    size_t  size;

    static const champ empty;

    champ(node_t* r, size_t sz)
        : root{r}, size{sz}
    {
    }

    champ(const champ& other)
        : champ{other.root, other.size}
    {
        inc();
    }

    champ(champ&& other)
        : champ{empty}
    {
        swap(*this, other);
    }

    champ& operator=(const champ& other)
    {
        auto next = other;
        swap(*this, next);
        return *this;
    }

    champ& operator=(champ&& other)
    {
        swap(*this, other);
        return *this;
    }

    friend void swap(champ& x, champ& y)
    {
        using std::swap;
        swap(x.root, y.root);
        swap(x.size, y.size);
    }

    ~champ()
    {
        dec();
    }

    void inc() const
    {
        root->inc();
    }

    void dec() const
    {
        dec_traversal(root, 0);
    }

    void dec_traversal(node_t* node, count_t depth) const
    {
        if (node->dec()) {
            if (depth < max_depth<B>) {
                auto fst = node->children();
                auto lst = fst + popcount(node->nodemap());
                for (; fst != lst; ++fst)
                    dec_traversal(*fst, depth + 1);
                node_t::delete_inner(node);
            } else {
                node_t::delete_collision(node);
            }
        }
    }

    template <typename K>
    const T* get(const K& k) const
    {
        auto node = root;
        auto hash = Hash{}(k);
        for (auto i = count_t{}; i < max_depth<B>; ++i) {
            auto idx = hash & mask<B>;
            if (node->nodemap() & idx) {
                auto offset = popcount(node->nodemap() & (idx - 1));
                node = node->children() [offset];
                hash = hash >> B;
            } else if (node->datamap() & idx) {
                auto offset = popcount(node->datamap() & (idx - 1));
                auto val    = node->values() + offset;
                if (Equal{}(*val, k))
                    return val;
                else
                    return nullptr;
            } else {
                return nullptr;
            }
        }
        auto fst = node->collisions();
        auto lst = fst + node->collision_count();
        for (; fst != lst; ++fst)
            if (Equal{}(*fst, k))
                return fst;
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
