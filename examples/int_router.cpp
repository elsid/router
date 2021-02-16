#include <iostream>
#include <span>
#include <type_traits>
#include <vector>

#include <router/router.hpp>

namespace {

using router::Selector;
using router::Action;

inline int increment(int value) {
    return value + 1;
}

inline int decrement(int value) {
    return value - 1;
}

constexpr Selector dispatch_impl(
    Action(std::integral_constant<int, 3457384>{}, [] (int v) { return increment(v); }),
    Action(std::integral_constant<int, 5438990>{}, [] (int v) { return decrement(v); })
);

inline int dispatch(std::span<int> input) {
    int result;
    dispatch_impl(input)
            .map([&] (int v) { result = v; })
            .map_error([] (auto) { std::abort(); });
    return result;
}

} // namespace

int main() {
    std::vector a {3457384, 42};
    std::vector b {5438990, 13};
    std::cout << dispatch(a) << std::endl;
    std::cout << dispatch(b) << std::endl;
    return 0;
}
