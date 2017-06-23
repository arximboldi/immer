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

#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

#include <immer/vector.hpp>
#include <immer/refcount/unsafe_refcount_policy.hpp>

namespace {

struct heap_t
{
    template <typename ...Tags>
    static void* allocate(std::size_t size, Tags...)
    {
        return PyMem_Malloc(size);
    }

    template <typename ...Tags>
    static void deallocate(std::size_t, void* obj, Tags...)
    {
        PyMem_Free(obj);
    }
};

using memory_t = immer::memory_policy<
    immer::unsafe_free_list_heap_policy<heap_t>,
    immer::unsafe_refcount_policy>;

template <typename Vector>
struct immer_vector_indexing_suite : boost::python::vector_indexing_suite<
    Vector, true, immer_vector_indexing_suite<Vector>>
{
    using value_t = typename Vector::value_type;
    using index_t = typename Vector::size_type;
    using object_t = boost::python::object;

    static void forbidden() { throw std::runtime_error{"immutable!"}; }
    static void todo() { throw std::runtime_error{"TODO!"}; }

    static const value_t& get_item(const Vector& v, index_t i) { return v[i]; }
    static object_t get_slice(const Vector&, index_t, index_t) { todo(); }

    static void set_item(const Vector&, index_t, const value_t&) { forbidden(); }
    static void delete_item(const Vector&, index_t) { forbidden(); }
    static void set_slice(const Vector&, index_t, index_t, const value_t&) { forbidden(); }
    static void delete_slice(const Vector&, index_t, index_t) { forbidden(); }
    template <typename Iter>
    static void set_slice(const Vector&, index_t, index_t, Iter, Iter) { forbidden(); }
    template <class Iter>
    static void extend(const Vector& container, Iter, Iter) { forbidden(); }
};

} // anonymous namespace


BOOST_PYTHON_MODULE(immer_python_module)
{
    using namespace boost::python;

    using vector_t = immer::vector<object, memory_t>;

    class_<vector_t>("Vector")
        .def(immer_vector_indexing_suite<vector_t>())
        .def("append",
             +[] (const vector_t& v, object x) {
                 return v.push_back(std::move(x));
              })
        .def("set",
             +[] (const vector_t& v, std::size_t i, object x) {
                 return v.set(i, std::move(x));
              })
        ;
}
