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

#include <pybind11/pybind11.h>

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

} // anonymous namespace

namespace py = pybind11;

PYBIND11_PLUGIN(immer_python_module)
{
    py::module m("immer", R"pbdoc(
        Immer
        -----
        .. currentmodule:: immer
        .. autosummary::
           :toctree: _generate
           Vector
    )pbdoc");

    using vector_t = immer::vector<py::object, memory_t>;

    py::class_<vector_t>(m, "Vector")
        .def(py::init<>())
        .def("__len__", &vector_t::size)
        .def("__getitem__",
             [] (const vector_t& v, std::size_t i) {
                 if (i > v.size())
                     throw py::index_error{"Index out of range"};
                 return v[i];
             })
        .def("append",
             [] (const vector_t& v, py::object x) {
                 return v.push_back(std::move(x));
             })
        .def("set",
             [] (const vector_t& v, std::size_t i, py::object x) {
                 return v.set(i, std::move(x));
             });

#ifdef VERSION_INFO
    m.attr("__version__") = py::str(VERSION_INFO);
#else
    m.attr("__version__") = py::str("dev");
#endif

    return m.ptr();
}
