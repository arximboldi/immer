
#include <immer/vector.hpp>
#include <immer/refcount/unsafe_refcount_policy.hpp>
#include <emscripten/bind.h>

namespace {

struct single_thread_heap_policy
{
    template <std::size_t... Sizes>
    struct apply
    {
        using type = immer::with_free_list_node<
            immer::unsafe_free_list_heap<
                std::max({Sizes...}),
                immer::malloc_heap>>;
    };
};

template <typename T>
void bind_vector(const char* name)
{
    using emscripten::class_;

    using memory_t = immer::memory_policy<
        single_thread_heap_policy,
        immer::unsafe_refcount_policy>;

    using vector_t = immer::vector<T, 5, memory_t>;

    class_<vector_t>(name)
        .constructor()
        .function("push",  &vector_t::push_back)
        .function("set",   &vector_t::assoc)
        .function("get",   &vector_t::operator[])
        .property("size",  &vector_t::size);
}

} // anonymous namespace

EMSCRIPTEN_BINDINGS(immer)
{
    bind_vector<emscripten::val>("Vector");
    bind_vector<int>("VectorInt");
    bind_vector<double>("VectorNumber");
}
