
#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

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
using immer_vector_t = immer::vector<T, memory_t>;

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


BOOST_PYTHON_MODULE(immer)
{
    using namespace boost::python;

    using vector_t = immer_vector_t<object>;

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
