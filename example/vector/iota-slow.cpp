
#include <immer/vector.hpp>
#include <iostream>

// include:myiota/start
immer::vector<int> myiota(immer::vector<int> v, int first, int last)
{
    for (auto i = first; i < last; ++i)
        v = v.push_back(i);
    return v;
}
// include:myiota/end

int main()
{
    auto v = myiota({}, 0, 100);
    std::copy(v.begin(), v.end(),
              std::ostream_iterator<int>{
                  std::cout, "\n"});
}
