#include <immer/vector.hpp>
#include <cassert>

// include:move-bad/start
immer::vector<int> do_stuff(const immer::vector<int> v)
{
    return std::move(v).push_back(42);
}
// include:move-bad/end

// include:move-good/start
immer::vector<int> do_stuff_better(immer::vector<int> v)
{
    return std::move(v).push_back(42);
}
// include:move-good/end

int main()
{
    auto v = immer::vector<int>{};
    auto v1 = do_stuff(v);
    auto v2 = do_stuff_better(v);
    assert(v1.size() == 1);
    assert(v2.size() == 1);
    assert(v1[0] == 42);
    assert(v2[0] == 42);
}
