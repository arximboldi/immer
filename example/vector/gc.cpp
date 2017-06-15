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

// include:example/start
#include <immer/heap/gc_heap.hpp>
#include <immer/heap/heap_policy.hpp>
#include <immer/refcount/no_refcount_policy.hpp>
#include <immer/memory_policy.hpp>
#include <immer/vector.hpp>

#include <iostream>

// declare a memory policy for using a tracing garbage collector
using gc_policy = immer::memory_policy<
    immer::heap_policy<immer::gc_heap>,
    immer::no_refcount_policy,
    immer::gc_transience_policy,
    false>;

// alias the vector type so we are not concerned about memory policies
// in the places where we actually use it
template <typename T>
using my_vector = immer::vector<T, gc_policy>;

int main()
{
    auto v = my_vector<const char*>()
        .push_back("hello, ")
        .push_back("world!\n");

    for (auto s : v) std::cout << s;
}
// include:example/end
