#include <immer/vector.hpp>
int main()
{
    const auto v0 = immer::vector<int>{};
    const auto v1 = v0.push_back(13);
    assert(v0.size() == 0 && v1.size() == 1 && v1[0] == 13);

    const auto v2 = v1.set(0, 42);
    assert(v1[0] == 13 && v2[0] == 42);
}
