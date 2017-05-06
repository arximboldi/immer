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

#include <immer/vector.hpp>
#include <immer/refcount/unsafe_refcount_policy.hpp>
#include <emscripten/bind.h>

namespace {

using memory_t = immer::memory_policy<
    immer::unsafe_free_list_heap_policy<immer::malloc_heap>,
    immer::unsafe_refcount_policy>;

template <typename T>
using js_vector_t = immer::vector<T, memory_t>;

template <typename VectorT>
VectorT range(typename VectorT::value_type first,
              typename VectorT::value_type last)
{
    auto v = VectorT{};
    for (; first != last; ++first)
        v = std::move(v).push_back(first);
    return v;
}

template <typename VectorT>
VectorT range_slow(typename VectorT::value_type first,
                   typename VectorT::value_type last)
{
    auto v = VectorT{};
    for (; first != last; ++first)
        v = v.push_back(first);
    return v;
}

template <typename VectorT>
VectorT push_back(VectorT& v, typename VectorT::value_type x)
{ return v.push_back(x); }

template <typename VectorT>
VectorT set(VectorT& v, std::size_t i, typename VectorT::value_type x)
{ return v.set(i, x); }

template <typename T>
void bind_vector(const char* name)
{
    using emscripten::class_;

    using vector_t = js_vector_t<T>;

    class_<vector_t>(name)
        .constructor()
        .function("push",  &push_back<vector_t>)
        .function("set",   &set<vector_t>)
        .function("get",   &vector_t::operator[])
        .property("size",  &vector_t::size);
}

} // anonymous namespace

EMSCRIPTEN_BINDINGS(immer)
{
    using emscripten::function;

    bind_vector<emscripten::val>("Vector");
    bind_vector<int>("VectorInt");
    bind_vector<double>("VectorNumber");

    function("range_int", &range<js_vector_t<int>>);
    function("rangeSlow_int", &range_slow<js_vector_t<int>>);
    function("range_double", &range<js_vector_t<double>>);
    function("rangeSlow_double", &range_slow<js_vector_t<double>>);
}
