
#include <immu/detail/ref_count_base.hpp>
#include <boost/intrusive_ptr.hpp>
#include <vector>

namespace immu {

namespace detail {
namespace ivektor {

template <typename T>
struct data : std::vector<T>, ref_count_base<data<T>>
{
    using std::vector<T>::vector;

    data(const data& v) : std::vector<T> (v) {}
    data(data&& v) : std::vector<T> (std::move(v)) {}
};

template <typename T>
struct impl
{
    const T& get(std::size_t index) const
    {
        return (*d)[index];
    }

    impl push_back(T value) const
    {
        return mutated([&] (auto& x) {
            x.push_back(std::move(value));
        });
    }

    impl assoc(std::size_t idx, T value) const
    {
        return mutated([&] (auto& x) {
            x[idx] = std::move(value);
        });
    }

    template <typename Fn>
    impl update(std::size_t idx, Fn&& op) const
    {
        return mutated([&] (auto& x) {
            x[idx] = std::forward<Fn>(op) (std::move(x[idx]));
        });
    }

    template <typename Fn>
    impl mutated(Fn&& op) const
    {
        auto x = new data<T>(*d);
        std::forward<Fn>(op) (*x);
        return { x };
    }

    boost::intrusive_ptr<data<T>> d;
};

template <typename T> const impl<T> empty = { new data<T>{} };

} // namespace ivektor
} // namespace detail

} // namespace immu
