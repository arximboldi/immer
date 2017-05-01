#include <immer/box.hpp>
#include <cassert>
#include <string>

int main()
{
    {
// include:update/start
        auto v1 = immer::box<std::string>{"hello"};
        auto v2 = v1.update([&] (auto l) {
            return l + ", world!";
        });

        assert((v1 == immer::box<std::string>{"hello"}));
        assert((v2 == immer::box<std::string>{"hello, world!"}));
// include:update/end
    }
}
