
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

    static bool dec(const data*) { return false; };

    static void dec_unsafe(const data*) {};
};

} // namespace immu
