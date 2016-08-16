
#pragma once

#include <immu/refcount/no_refcount_policy.hpp>

namespace immu {

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

} // namespace immu
