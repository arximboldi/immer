
#pragma once

namespace immu {

struct disowned {};

struct no_refcount_policy
{
    static constexpr bool enabled = false;

    struct data
    {
        data() {};
        data(disowned) {}
    };

    static void inc(const data*) {};

    template <typename Fn>
    static void dec(const data*, Fn&&) {};

    static void dec_unsafe(const data*) {};
};

} // namespace immu
