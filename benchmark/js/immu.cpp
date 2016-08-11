
#include <immu/vektor.hpp>
#include <emscripten/bind.h>

namespace {

// default constructible val
struct val : emscripten::val
{
    using base_t = emscripten::val;
    using base_t::base_t;

    val() : base_t{emscripten::val::undefined()} {}
    val(emscripten::val v) : base_t{v} {}
};

immu::vektor<val> push(const immu::vektor<val>& self,
                       emscripten::val x)
{ return self.push_back(std::move(x)); }

immu::vektor<val> set(const immu::vektor<val>& self,
                      std::size_t idx,
                      emscripten::val x)
{ return self.assoc(idx, std::move(x)); }

emscripten::val get(const immu::vektor<val>& self,
                    std::size_t idx)
{ return self[idx]; }

std::size_t size(const immu::vektor<val>& self)
{ return self.size(); }

} // anon namespace


EMSCRIPTEN_BINDINGS(immu)
{
    using emscripten::class_;

    class_<immu::vektor<val>>("Vektor")
        .constructor<>()
        .function("push",  push)
        .function("set",   set)
        .function("get",   get)
        .property("size",  size)
        ;

    class_<immu::vektor<int>>("VektorInt")
        .constructor<>()
        .function("push",  &immu::vektor<int>::push_back)
        .function("set",   &immu::vektor<int>::assoc)
        .function("get",   &immu::vektor<int>::operator[])
        .property("size",  &immu::vektor<int>::size)
        ;
}
