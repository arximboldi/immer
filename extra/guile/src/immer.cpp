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

#include <immer/flex_vector.hpp>
#include <immer/flex_vector_transient.hpp>
#include <immer/algorithm.hpp>
#include <scm/scm.hpp>
#include <iostream>

namespace {

struct guile_heap
{
    static void* allocate(std::size_t size)
    { return scm_gc_malloc(size, "immer"); }

    static void* allocate(std::size_t size, immer::norefs_tag)
    { return scm_gc_malloc_pointerless(size, "immer"); }

    template <typename ...Tags>
    static void deallocate(std::size_t size, void* obj, Tags...)
    { scm_gc_free(obj, size, "immer"); }
};

using guile_memory = immer::memory_policy<
    immer::heap_policy<guile_heap>,
    immer::no_refcount_policy,
    immer::gc_transience_policy,
    false>;

template <typename T>
using guile_ivector = immer::flex_vector<T, guile_memory>;

} // anonymous namespace

SCM_DECLARE_FOREIGN_TYPE(guile_ivector<scm::val>);

extern "C"
void init_immer()
{
    using self_t = guile_ivector<scm::val>;
    using size_t = typename self_t::size_type;

    scm::type<self_t>("ivector")
        .constructor([] (scm::args rest) {
            return self_t(rest.begin(), rest.end());
        })
        .maker([] (size_t n, scm::args rest) {
            return self_t(n, rest ? *rest : scm::val{});
        })
        .define("ref", &self_t::operator[])
        .define("length", &self_t::size)
        .define("set", [] (const self_t& v, size_t i, scm::val x) {
            return v.set(i, x);
        })
        .define("update", [] (const self_t& v, size_t i, scm::val fn) {
            return v.update(i, fn);
        })
        .define("push", [] (const self_t& v, scm::val x) {
            return v.push_back(x);
        })
        .define("take", [] (const self_t& v, size_t s) {
            return v.take(s);
        })
        .define("drop", [] (const self_t& v, size_t s) {
            return v.drop(s);
        })
        .define("append", [] (self_t v, scm::args rest) {
            for (auto x : rest)
                v = v + x;
            return v;
        })
        .define("fold", [] (scm::val fn, scm::val first, const self_t& v) {
            return immer::accumulate(v, first, fn);
        })
        ;
}
