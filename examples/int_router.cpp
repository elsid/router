#include <algorithm>
#include <iostream>
#include <iterator>
#include <ranges>
#include <span>
#include <type_traits>
#include <variant>
#include <vector>

namespace router {

template <class Tag, class F>
struct Action {
    static constexpr typename Tag::value_type name = Tag::value;

    F f;

    template <class T>
    constexpr explicit Action(Tag, T&& f) : f(std::forward<T>(f)) {}

    template <class ... Args>
    auto operator ()(Args&& ... args) const -> std::invoke_result_t<F, Args&& ...> {
        return std::invoke(f, std::forward<Args>(args) ...);
    }
};

template <class Tag, class T>
Action(Tag, T&& f) -> Action<Tag, std::decay_t<T>>;

template <class F>
struct ArgumentsNumber {
    static constexpr std::size_t value = ArgumentsNumber<decltype(&F::operator())>::value - 1;
};

template <class R, class ... Args>
struct ArgumentsNumber<R (*)(Args ...)>
    : std::integral_constant<std::size_t, sizeof ... (Args)> {};

template <class T, class R, class ... Args>
struct ArgumentsNumber<R (T::*)(Args ...)>
    : std::integral_constant<std::size_t, sizeof ... (Args) + 1> {};

template <class T, class R, class ... Args>
struct ArgumentsNumber<R (T::*)(Args ...) const>
    : std::integral_constant<std::size_t, sizeof ... (Args) + 1> {};

template <class Tag, class F>
struct ArgumentsNumber<Action<Tag, F>> : ArgumentsNumber<F> {};

template <class F>
constexpr std::size_t arguments_number_v = ArgumentsNumber<F>::value;

template <class F>
struct ReturnType {
    using type = typename ReturnType<decltype(&F::operator())>::type;
};

template <class T, class R, class ... Args>
struct ReturnType<R (T::*)(Args ...)> {
    using type = R;
};

template <class T, class R, class ... Args>
struct ReturnType<R (T::*)(Args ...) const> {
    using type = R;
};

template <class R, class ... Args>
struct ReturnType<R (*)(Args ...)> {
    using type = R;
};

template <class Tag, class F>
struct ReturnType<Action<Tag, F>> {
    using type = typename ReturnType<F>::type;
};

template <class F>
using return_type_t = typename ReturnType<F>::type;

template <class ... Ts>
struct Visited {};

template <class ... Ts>
struct IsVisited : std::false_type {};

template <class ... Ts>
constexpr bool is_visited_v = IsVisited<Ts ...>::value;

template <class T, class H, class ... Ts>
struct IsVisited<T, Visited<H, Ts ...>> : IsVisited<T, Visited<Ts ...>> {};

template <class T, class ... Ts>
struct IsVisited<T, Visited<T, Ts ...>> : std::true_type {};

template <class T>
struct IsVisited<T, Visited<>> : std::false_type {};

template <class ... Ts>
struct Distinct : Distinct<Visited<>, Ts ...> {};

template <class T, class ... Vs, class ... Ts>
struct Distinct<Visited<Vs ...>, T, Ts ...>
        : std::conditional_t<
            is_visited_v<T, Visited<Vs ...>>,
            Distinct<Visited<Vs ...>, Ts ...>,
            Distinct<Visited<T, Vs ...>, Ts ...>
        > {};

template <class ... Vs, class ... Qs, class ... Ts>
struct Distinct<Visited<Vs ...>, std::variant<Qs ...>, Ts ...>
        : Distinct<Visited<Vs ...>, Qs ..., Ts ...> {};

template <class ... Ts>
struct Distinct<Visited<Ts ...>> {
    using type = std::variant<Ts ...>;
};

template <class T>
struct Distinct<Visited<T>> {
    using type = T;
};

template <class ... Ts>
using distinct_t = typename Distinct<Ts ...>::type;

inline auto consume(std::ranges::input_range auto input) {
    return std::ranges::subrange(std::next(std::begin(input)), std::end(input));
}

template <class Action, std::ranges::input_range Range, class ... Args>
auto invoke(const Action& action, Range input, Args&& ... args) {
    if constexpr (std::is_invocable_v<Action, Range, Args&& ...>) {
        return action(input, std::forward<Args>(args) ...);
    } else if constexpr (sizeof ... (Args) >= arguments_number_v<Action>) {
        if (!std::empty(input)) {
            std::abort();
        }
        return action(std::forward<Args>(args) ...);
    } else {
        if (std::empty(input)) {
            std::abort();
        }
        return invoke(action, consume(input), std::forward<Args>(args) ..., *std::begin(input));
    }
}

template <class, class = std::void_t<>>
struct HasName : std::false_type {};

template <class T>
struct HasName<T, std::void_t<decltype(T::name)>> : std::true_type {};

template <class T>
constexpr bool has_name_v = HasName<T>::value;

template <class ... Actions>
struct Selector {
    using return_type = distinct_t<return_type_t<Actions> ...>;

    const std::tuple<Actions ...> actions;

    template <class ... Ts>
    constexpr explicit Selector(Ts&& ... actions) : actions(std::forward<Ts>(actions) ...) {}

    template <class ... Args>
    return_type operator ()(std::ranges::input_range auto input, Args&& ... args) const {
        if (std::empty(input)) {
            std::abort();
        }
        return find_action(input, [&] (std::ranges::input_range auto input, const auto& action) {
            return invoke(action, input, std::forward<Args>(args) ...);
        });
    }

    template <std::size_t i = 0, class F>
    return_type find_action(std::ranges::input_range auto input, F&& f) const {
        if constexpr (i >= std::tuple_size_v<decltype(actions)>) {
            std::abort();
            return return_type {};
        } else if constexpr (has_name_v<std::tuple_element_t<i, decltype(actions)>>) {
            if (std::get<i>(actions).name == *std::begin(input)) {
                return make_result(f(consume(input), std::get<i>(actions)));
            }
            return find_action<i + 1>(input, std::forward<F>(f));
        } else {
            return make_result(f(input, std::get<i>(actions)));
        }
    }

    template <class ... Ts>
    static auto make_result(std::variant<Ts ...>&& value) {
        return std::visit(
            [] (auto&& v) { return return_type(std::move(v)); },
            std::move(value)
        );
    }

    template <class T>
    static auto make_result(T&& value) {
        return return_type(std::forward<T>(value));
    }
};

template <class ... Actions>
Selector(Actions&& ...) -> Selector<std::decay_t<Actions> ...>;

template <class Tag, class ... Actions>
struct ReturnType<Selector<Tag, Actions ...>> {
    using type = typename Selector<Tag, Actions ...>::return_type;
};

template <class T, class ... Actions>
struct Argument {
    Selector<Actions ...> selector;

    template <class ... F>
    constexpr explicit Argument(F&& ... f) : selector(std::forward<F>(f) ...) {}

    template <std::ranges::input_range Range, class ... Args>
    auto operator ()(Range input, Args&& ... args) const {
        if (std::empty(input)) {
            std::abort();
        }
        return selector(consume(input), std::forward<Args>(args) ..., T {*std::begin(input)});
    }
};

template <class T, class ... Actions>
constexpr auto argument(Actions&& ... actions) {
    return Argument<T, std::decay_t<Actions> ...>(std::forward<Actions>(actions) ...);
}

template <class T, class ... Actions>
struct ArgumentsNumber<Argument<T, Actions ...>> : ArgumentsNumber<Selector<Actions ...>> {};

template <class T, class ... Actions>
struct ReturnType<Argument<T, Actions ...>> : ReturnType<Selector<Actions ...>> {};

} // namespace router

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
    return dispatch_impl(input);
}

} // namespace

int main() {
    std::vector a {3457384, 42};
    std::vector b {5438990, 13};
    std::cout << dispatch(a) << std::endl;
    std::cout << dispatch(b) << std::endl;
    return 0;
}
