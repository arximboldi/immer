
#include <immu/vektor.hpp>
#include <doctest.h>

using namespace immu;

TEST_CASE("instantiation")
{
    auto v = vektor<int>{};
    CHECK(v.size() == 0);
}
