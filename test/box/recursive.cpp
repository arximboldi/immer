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

#include <immer/box.hpp>
#include <immer/vector.hpp>
#include <immer/flex_vector.hpp>
#include <immer/map.hpp>
#include <immer/set.hpp>
#include <immer/algorithm.hpp>

#include <catch.hpp>

struct rec_vec
{
    int data;
    immer::vector<immer::box<rec_vec>> children;
};

TEST_CASE("recursive vector")
{
    auto v1 = rec_vec{42, {rec_vec{12, {}},
                           rec_vec{13, {rec_vec{5, {}},
                                        rec_vec{7, {}}}},
                           rec_vec{15, {}}}};
    CHECK(v1.data == 42);
    CHECK(v1.children[0]->data == 12);
    CHECK(v1.children[1]->children[0]->data == 5);
}

struct rec_fvec
{
    int data;
    immer::flex_vector<immer::box<rec_fvec>> children;
};

TEST_CASE("recursive flex_vector")
{
    auto v1 = rec_fvec{42, {rec_fvec{12, {}},
                            rec_fvec{13, {rec_fvec{5, {}},
                                          rec_fvec{7, {}}}},
                            rec_fvec{15, {}}}};
    CHECK(v1.data == 42);
    CHECK(v1.children[0]->data == 12);
    CHECK(v1.children[1]->children[0]->data == 5);
}

struct rec_map
{
    int data;
    immer::map<std::string, immer::box<rec_map>> children;
};

TEST_CASE("recursive map")
{
    auto v1 = rec_map{42, {}};
    auto v2 = rec_map{43, v1.children.set("hello", rec_map{12, {}})};
    auto v3 = rec_map{44, v2.children.set("world", rec_map{13, {}})};

    CHECK(v3.data == 44);
    CHECK(v3.children["hello"]->data == 12);
    CHECK(v3.children["world"]->data == 13);
}

struct rec_set
{
    int data;
    immer::set<immer::box<rec_set>> children;

    bool operator==(const rec_set& other) const
    { return data == other.data && children == other.children; }
    bool operator!=(const rec_set& other) const
    { return !(*this == other); }
};

namespace std {

template <>
struct hash<rec_set>
{
    auto operator() (const rec_set& s)
    {
        return std::hash<decltype(s.data)>{}(s.data) ^
               immer::accumulate(s.children, std::size_t{}, [] (auto ac, auto v) {
                   return std::hash<decltype(v)>{}(v);
               });
    }
};

} // namespace std

TEST_CASE("recursive set")
{
    auto v1 = rec_set{42, {}};
    auto v2 = rec_set{43, v1.children.insert(rec_set{12, {}})};
    auto v3 = rec_set{44, v2.children.insert(rec_set{13, {}})};

    CHECK(v3.data == 44);
    CHECK(v3.children.count(rec_set{12, {}}) == 1);
    CHECK(v3.children.count(rec_set{13, {}}) == 1);
    CHECK(v3.children.count(rec_set{14, {}}) == 0);
}
