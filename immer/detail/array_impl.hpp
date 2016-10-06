
#include <immer/detail/ref_count_base.hpp>
#include <boost/intrusive_ptr.hpp>
#include <vector>

namespace immer {
namespace detail {

template <typename T>
struct array_data : std::vector<T>, ref_count_base<array_data<T>>
{
    using std::vector<T>::vector;

    array_data(const array_data& v) : std::vector<T> (v) {}
    array_data(array_data&& v) : std::vector<T> (std::move(v)) {}
};

template <typename T>
struct array_impl
{
    static const array_impl empty;

    const T& get(std::size_t index) const
    {
        return (*d)[index];
    }

    array_impl push_back(T value) const
    {
        return mutated([&] (auto& x) {
            x.push_back(std::move(value));
        });
    }

    array_impl assoc(std::size_t idx, T value) const
    {
        return mutated([&] (auto& x) {
            x[idx] = std::move(value);
        });
    }

    template <typename Fn>
    array_impl update(std::size_t idx, Fn&& op) const
    {
        return mutated([&] (auto& x) {
            x[idx] = std::forward<Fn>(op) (std::move(x[idx]));
        });
    }

    template <typename Fn>
    array_impl mutated(Fn&& op) const
    {
        auto x = new array_data<T>(*d);
        std::forward<Fn>(op) (*x);
        return { x };
    }

    boost::intrusive_ptr<array_data<T>> d;
};

template <typename T>
const array_impl<T> array_impl<T>::empty = { new array_data<T>{} };

} // namespace detail
} // namespace immer
