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
#include "scm.hpp"
#include <iostream>

namespace {

struct guile_heap
{
    static void* allocate(std::size_t size)
    {
        return scm_gc_malloc(size, "immer");
    }

    static void* allocate(std::size_t size, immer::norefs_tag)
    {
        return scm_gc_malloc_pointerless(size, "immer");
    }

    template <typename ...Tags>
    static void deallocate(std::size_t size, void* obj, Tags...)
    {
        scm_gc_free(obj, size, "immer");
    }
};

using guile_memory = immer::memory_policy<
    immer::heap_policy<guile_heap>,
    immer::no_refcount_policy,
    immer::gc_transience_policy,
    false>;

template <typename T = SCM>
void init_vector(std::string prefix = "")
{
    using namespace std::string_literals;
    using ivector_t = immer::flex_vector<T, guile_memory>;
    using tvector_t = typename ivector_t::transient_type;
    using size_t    = typename ivector_t::size_type;

    static auto iname = "ivector" + prefix;
    static auto ivector_type =
        scm_make_foreign_object_type(
            scm_from_utf8_symbol(iname.c_str()),
            scm_list_1(scm_from_utf8_symbol("data")),
            nullptr);

    static auto iname_make = iname;
    scm_c_define_gsubr(
        iname_make.c_str(), 0, 0, 1,
        (scm_t_subr) +[] (SCM rest)
        {
            auto t = tvector_t{};
            for (; !scm_is_null(rest); rest = scm_cdr(rest))
                t.push_back(scm::to_cpp<T>(scm_car(rest)));
            return scm_make_foreign_object_1(
                ivector_type,
                new (scm_gc_malloc(sizeof(ivector_t), "immer"))
                ivector_t{t.persistent()});
        });
    scm_c_export(iname_make.c_str());

    static auto iname_ref = iname + "-ref"s;
    scm_c_define_gsubr(
        iname_ref.c_str(), 2, 0, 0,
        (scm_t_subr) +[] (SCM self, SCM index)
        {
            scm_assert_foreign_object_type(ivector_type, self);
            auto v = *(ivector_t*) scm_foreign_object_ref(self, 0);
            return scm::to_scm(v[scm::to_cpp<size_t>(index)]);
        });
    scm_c_export(iname_ref.c_str());

    static auto iname_length = iname + "-length"s;
    scm_c_define_gsubr(
        iname_length.c_str(), 1, 0, 0,
        (scm_t_subr) +[] (SCM self)
        {
            scm_assert_foreign_object_type(ivector_type, self);
            auto v = *(ivector_t*) scm_foreign_object_ref(self, 0);
            return scm::to_scm(v.size());
        });
    scm_c_export(iname_length.c_str());

    static auto iname_push = iname + "-push"s;
    scm_c_define_gsubr(
        iname_push.c_str(), 2, 0, 0,
        (scm_t_subr) +[] (SCM self, SCM value)
        {
            scm_assert_foreign_object_type(ivector_type, self);
            auto v = *(ivector_t*) scm_foreign_object_ref(self, 0);
            return scm_make_foreign_object_1(
                ivector_type,
                new (scm_gc_malloc(sizeof(ivector_t), "immer"))
                ivector_t{v.push_back(scm::to_cpp<T>(value))});
        });
    scm_c_export(iname_push.c_str());

    static auto iname_set = iname + "-set"s;
    scm_c_define_gsubr(
        iname_set.c_str(), 3, 0, 0,
        (scm_t_subr) +[] (SCM self, SCM index, SCM value)
        {
            scm_assert_foreign_object_type(ivector_type, self);
            auto v = *(ivector_t*) scm_foreign_object_ref(self, 0);
            return scm_make_foreign_object_1(
                ivector_type,
                new (scm_gc_malloc(sizeof(ivector_t), "immer"))
                ivector_t{v.set(scm::to_cpp<size_t>(index),
                                scm::to_cpp<T>(value))});
        });
    scm_c_export(iname_set.c_str());

    static auto iname_update = iname + "-update"s;
    scm_c_define_gsubr(
        iname_update.c_str(), 3, 0, 0,
        (scm_t_subr) +[] (SCM self, SCM index, SCM fn)
        {
            scm_assert_foreign_object_type(ivector_type, self);
            auto v = *(ivector_t*) scm_foreign_object_ref(self, 0);
            return scm_make_foreign_object_1(
                ivector_type,
                new (scm_gc_malloc(sizeof(ivector_t), "immer"))
                ivector_t{
                    v.update(
                        scm::to_cpp<size_t>(index),
                        [&] (auto value) {
                            return scm::to_cpp<T>(
                                scm_call_1(fn, scm::to_scm(value)));
                        })});
        });
    scm_c_export(iname_update.c_str());

    static auto iname_drop = iname + "-drop"s;
    scm_c_define_gsubr(
        iname_drop.c_str(), 2, 0, 0,
        (scm_t_subr) +[] (SCM self, SCM value)
        {
            scm_assert_foreign_object_type(ivector_type, self);
            auto v = *(ivector_t*) scm_foreign_object_ref(self, 0);
            return scm_make_foreign_object_1(
                ivector_type,
                new (scm_gc_malloc(sizeof(ivector_t), "immer"))
                ivector_t{v.drop(scm::to_cpp<size_t>(value))});
        });
    scm_c_export(iname_drop.c_str());

    static auto iname_take = iname + "-take"s;
    scm_c_define_gsubr(
        iname_take.c_str(), 2, 0, 0,
        (scm_t_subr) +[] (SCM self, SCM value)
        {
            scm_assert_foreign_object_type(ivector_type, self);
            auto v = *(ivector_t*) scm_foreign_object_ref(self, 0);
            return scm_make_foreign_object_1(
                ivector_type,
                new (scm_gc_malloc(sizeof(ivector_t), "immer"))
                ivector_t{v.take(scm::to_cpp<size_t>(value))});
        });
    scm_c_export(iname_take.c_str());

    static auto iname_append = iname + "-append"s;
    scm_c_define_gsubr(
        iname_append.c_str(), 1, 0, 1,
        (scm_t_subr) +[] (SCM self, SCM rest)
        {
            scm_assert_foreign_object_type(ivector_type, self);
            auto v = *(ivector_t*) scm_foreign_object_ref(self, 0);
            for (; !scm_is_null(rest); rest = scm_cdr(rest)) {
                auto next = scm_car(rest);
                scm_assert_foreign_object_type(ivector_type, next);
                v = v + *(ivector_t*) scm_foreign_object_ref(self, 0);
            }
            return scm_make_foreign_object_1(
                ivector_type,
                new (scm_gc_malloc(sizeof(ivector_t), "immer"))
                ivector_t{v});
        });
    scm_c_export(iname_append.c_str());

    static auto iname_fold = iname + "-fold"s;
    scm_c_define_gsubr(
        iname_fold.c_str(), 3, 0, 0,
        (scm_t_subr) +[] (SCM fn, SCM first, SCM self)
        {
            scm_assert_foreign_object_type(ivector_type, self);
            auto v = *(ivector_t*) scm_foreign_object_ref(self, 0);
            return immer::accumulate(v, first, [=] (auto acc, auto item) {
                return scm_call_2(fn, acc, scm::to_scm(item));
            });
        });
    scm_c_export(iname_fold.c_str());
}

} // anonymous namespace

extern "C"
void init_immer ()
{
    init_vector<SCM>("");
    init_vector<std::uint16_t>("-u8");
    init_vector<std::uint16_t>("-u16");
    init_vector<std::uint32_t>("-u32");
    init_vector<std::uint64_t>("-u64");
    init_vector<std::int16_t>("-s8");
    init_vector<std::int16_t>("-s16");
    init_vector<std::int32_t>("-s32");
    init_vector<std::int64_t>("-s64");
    init_vector<float>("-s32");
    init_vector<double>("-s64");
}
