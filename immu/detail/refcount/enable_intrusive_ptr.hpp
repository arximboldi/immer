
#pragma once

#include <immu/detail/refcount/no_refcount_policy.hpp>

namespace immu {
namespace detail {

template <typename Deriv, typename RefcountPolicy>
struct enable_intrusive_ptr : RefcountPolicy::data
{
    using policy = RefcountPolicy;

    enable_intrusive_ptr()
        : policy::data{disowned{}}
    {}

    friend void intrusive_ptr_add_ref(const Deriv* x)
    {
        policy::inc(x);
    }

    friend void intrusive_ptr_release(const Deriv* x)
    {
        policy::dec(x, [&] { delete x; });
    }
};

} // namespace deail
} // namespace immu
