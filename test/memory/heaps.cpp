//
// immer - immutable data structures for C++
// Copyright (C) 2016, 2017 Juan Pedro Bolivar Puente
//
// This file is part of immer.
//
// immer is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// immer is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with immer.  If not, see <http://www.gnu.org/licenses/>.
//

#include <immer/heap/malloc_heap.hpp>
#include <immer/heap/free_list_heap.hpp>
#include <immer/heap/thread_local_free_list_heap.hpp>
#include <immer/heap/gc_heap.hpp>

#include <catch.hpp>
#include <numeric>

// http://stackoverflow.com/questions/29322666/undefined-reference-to-cxa-thread-atexitcxxabi-when-compiling-with-libc#3043776
// https://reviews.llvm.org/D21803
#if IMMER_ENABLE_CXA_THREAD_ATEXIT_HACK
extern "C" int __cxa_thread_atexit(void (*func)(),
                                   void *obj,
                                   void *dso_symbol)
{
    return 0;
}
#endif

void do_stuff_to(void* buf, std::size_t size)
{
    auto ptr = static_cast<unsigned char*>(buf);
    CHECK(buf != 0);
    std::iota(ptr, ptr + size, 42);
}

TEST_CASE("malloc")
{
    using heap = immer::malloc_heap;

    SECTION("basic")
    {
        auto p = heap::allocate(42u);
        do_stuff_to(p, 42u);
        heap::deallocate(p);
    }
}

TEST_CASE("gc")
{
    using heap = immer::gc_heap;

    SECTION("basic")
    {
        auto p = heap::allocate(42u);
        do_stuff_to(p, 42u);
        heap::deallocate(p);
    }
}

template <typename Heap>
void test_free_list_heap()
{
    using heap = Heap;

    SECTION("basic")
    {
        auto p = heap::allocate(42u);
        do_stuff_to(p, 42u);
        heap::deallocate(p);
    }

    SECTION("reuse")
    {
        auto p = heap::allocate(42u);
        do_stuff_to(p, 42u);
        heap::deallocate(p);

        auto u = heap::allocate(12u);
        do_stuff_to(u, 12u);
        heap::deallocate(u);
        CHECK(u == p);
    }
}

TEST_CASE("free list")
{
    test_free_list_heap<immer::free_list_heap<42u, 2, immer::malloc_heap>>();
}

TEST_CASE("thread local free list")
{
    test_free_list_heap<immer::thread_local_free_list_heap<42u, 2, immer::malloc_heap>>();
}


TEST_CASE("unsafe free_list")
{
    test_free_list_heap<immer::unsafe_free_list_heap<42u, 2, immer::malloc_heap>>();
}
