
#include <pybind11/pybind11.h>

#include <immer/vector.hpp>
#include <immer/refcount/unsafe_refcount_policy.hpp>

namespace {

struct python_heap
{
    template <typename ...Tags>
    static void* allocate(std::size_t size,  Tags...)
    {
        return PyMem_Malloc(size);
    }

    template <typename ...Tags>
    static void deallocate(void* obj)
    {
        PyMem_Free(obj);
    }
};

struct python_heap_policy
{
    template <std::size_t... Sizes>
    struct apply
    {
        using type = immer::with_free_list_node<
            immer::unsafe_free_list_heap<
                std::max({Sizes...}),
                python_heap>>;
    };
};

using memory_t = immer::memory_policy<
    python_heap_policy,
    immer::unsafe_refcount_policy>;

template <typename T>
using py_vector_t = immer::vector<T, memory_t>;

} // anonymous namespace

namespace py = pybind11;

PYBIND11_PLUGIN(immer)
{
    py::module m("immer", R"pbdoc(
        Immer
        -----
        .. currentmodule:: immer
        .. autosummary::
           :toctree: _generate
           Vector
    )pbdoc");

    using vector_t = py_vector_t<py::object>;

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
