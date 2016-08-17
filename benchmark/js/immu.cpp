
#include <immu/vektor.hpp>
#include <immu/refcount/unsafe_refcount_policy.hpp>
#include <emscripten/bind.h>

namespace {

struct single_thread_heap_policy
{
    template <std::size_t... Sizes>
    struct apply
    {
        using type = immu::with_free_list_node<
            immu::unsafe_free_list_heap<
                std::max({Sizes...}),
                immu::malloc_heap>>;
    };
};

template <typename T>
void bind_vektor(const char* name)
{
    using emscripten::class_;

    using memory_t = immu::memory_policy<
        single_thread_heap_policy,
        immu::unsafe_refcount_policy>;

    using vektor_t = immu::vektor<T, 5, memory_t>;

    class_<vektor_t>(name)
        .constructor()
        .function("push",  &vektor_t::push_back)
        .function("set",   &vektor_t::assoc)
        .function("get",   &vektor_t::operator[])
        .property("size",  &vektor_t::size);
}

} // anonymous namespace

EMSCRIPTEN_BINDINGS(immu)
{
    bind_vektor<emscripten::val>("Vektor");
    bind_vektor<int>("VektorInt");
    bind_vektor<double>("VektorNumber");
}
