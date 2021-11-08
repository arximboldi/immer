//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include <catch.hpp>
#include <immer/set.hpp>

using relocation_memory = immer::memory_policy<immer::default_heap_policy,
                                               immer::default_refcount_policy,
                                               immer::default_lock_policy,
                                               immer::no_transience_policy,
                                               true,
                                               false,
                                               true>;

template <typename T,
          typename Hash = std::hash<T>,
          typename Eq   = std::equal_to<T>>
using test_set_t = immer::set<T, Hash, Eq, relocation_memory, 3u>;

TEST_CASE("relocation info")
{
    auto v = test_set_t<unsigned>{};
    for (auto i = 0u; i < 512u; ++i)
        v = v.insert(i);
    auto iter                                    = v.begin();
    test_set_t<unsigned>::iterator::node_t *node = v.impl().root, *parent;
    unsigned depth                               = 0;

    while (node->nodemap()) {
        parent = node;
        node   = node->children()[0];
        ++depth;
    }
    CHECK(depth >= 2);
    for (auto i = 0u; i < parent->children_count(); ++i) {
        for (auto j = 0u; j < node->data_count(); ++j) {
            CHECK(iter.relocation_info().cur_off_ == j);
            CHECK(iter.relocation_info().path_off_[depth] == 0);
            CHECK(iter.relocation_info().path_off_[depth - 1] == i);
            CHECK(iter.relocation_info().path_off_[depth - 2] == 0);
            ++iter;
        }
        if (i + 1 < parent->children_count()) {
            node = parent->children()[i + 1];
        }
    }
}
