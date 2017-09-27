#pragma once

#include "dada.hpp"

namespace {

template <typename VP, typename VT>
struct transient_tester
{
    VP vp;
    VT vt;
    dadaism d = {};
    bool transient = false;

    transient_tester(VP vp)
        : vp{vp}
        , vt{vp.transient()}
    {}

    bool step()
    {
        auto s = d.next();
        if (soft_dada()) {
            auto new_transient = !transient;
            try {
                if (new_transient)
                    vt = vp.transient();
                else
                    vp = vt.persistent();
            } catch (const dada_error&) { return false; }
            transient = new_transient;
            return true;
        } else
            return false;
    }
};

template <typename VP>
transient_tester<VP, typename VP::transient_type>
as_transient_tester(VP p)
{
    return { std::move(p) };
}

} // anonymous namespace
