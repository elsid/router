#pragma once

#include <cstdlib>
#include <functional>
#include <ranges>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <variant>

#include <tl/expected.hpp>

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
inline constexpr std::size_t arguments_number_v = ArgumentsNumber<F>::value;

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
inline constexpr bool is_visited_v = IsVisited<Ts ...>::value;

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

enum class Errc {
    None,
    TooManyArguments,
    NotEnoughInput,
    InvalidAction,
};

template <class T>
using Result = tl::expected<T, Errc>;

template <class T>
struct ResultValue {
    using type = T;
};

template <class T>
struct ResultValue<Result<T>> {
    using type = typename ResultValue<T>::type;
};

template <class T>
using result_value_t = typename ResultValue<T>::type;

template <class ReturnType>
struct MakeResult {
    template <class T>
    ReturnType operator ()(Result<T>&& value) const {
        return std::move(value).and_then(*this);
    }

    template <class ... Ts>
    ReturnType operator ()(std::variant<Ts ...>&& value) const {
        return std::visit(
            [] (auto&& v) { return ReturnType(std::move(v)); },
            std::move(value)
        );
    }

    template <class T>
    ReturnType operator ()(T&& value) const {
        return std::forward<T>(value);
    }
};

template <class Action, std::ranges::input_range Range, class ... Args>
inline auto invoke(const Action& action, Range input, Args&& ... args) {
    if constexpr (std::is_invocable_v<Action, Range, Args&& ...>) {
        using Value = decltype(action(input, std::forward<Args>(args) ...));
        return Result<Value>(action(input, std::forward<Args>(args) ...));
    } else if constexpr (sizeof ... (Args) >= arguments_number_v<Action>) {
        using Value = decltype(action(std::forward<Args>(args) ...));
        if (!std::empty(input)) {
            return Result<Value>(tl::make_unexpected(Errc::TooManyArguments));
        }
        return Result<Value>(action(std::forward<Args>(args) ...));
    } else {
        using Value = decltype(invoke(action, consume(input), std::forward<Args>(args) ..., *std::begin(input)));
        if (std::empty(input)) {
            return Result<Value>(tl::make_unexpected(Errc::NotEnoughInput));
        }
        return Result<Value>(invoke(action, consume(input), std::forward<Args>(args) ..., *std::begin(input)));
    }
}

template <class, class = std::void_t<>>
struct HasName : std::false_type {};

template <class T>
struct HasName<T, std::void_t<decltype(T::name)>> : std::true_type {};

template <class T>
inline constexpr bool has_name_v = HasName<T>::value;

template <class ... Actions>
struct Selector {
    using return_type = Result<distinct_t<result_value_t<return_type_t<Actions>> ...>>;

    static constexpr MakeResult<return_type> make_result {};

    const std::tuple<Actions ...> actions;

    template <class ... Ts>
    constexpr explicit Selector(Ts&& ... actions) : actions(std::forward<Ts>(actions) ...) {}

    template <class ... Args>
    return_type operator ()(std::ranges::input_range auto input, Args&& ... args) const {
        if (std::empty(input)) {
            return tl::make_unexpected(Errc::NotEnoughInput);
        }
        return find_action(input, [&] (std::ranges::input_range auto input, const auto& action) {
            return invoke(action, input, std::forward<Args>(args) ...);
        });
    }

    template <std::size_t i = 0, class F>
    return_type find_action(std::ranges::input_range auto input, F&& f) const {
        if constexpr (i >= std::tuple_size_v<decltype(actions)>) {
            return tl::make_unexpected(Errc::InvalidAction);
        } else if constexpr (has_name_v<std::tuple_element_t<i, decltype(actions)>>) {
            if (std::get<i>(actions).name == *std::begin(input)) {
                return make_result(f(consume(input), std::get<i>(actions)));
            }
            return find_action<i + 1>(input, std::forward<F>(f));
        } else {
            return make_result(f(input, std::get<i>(actions)));
        }
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

    template <class ... Args>
    auto operator ()(std::ranges::input_range auto input, Args&& ... args) const
        -> typename Selector<Actions ...>::return_type {
        if (std::empty(input)) {
            return tl::make_unexpected(Errc::NotEnoughInput);
        }
        return selector(consume(input), std::forward<Args>(args) ..., T {*std::begin(input)});
    }
};

template <class T, class ... Actions>
inline constexpr auto argument(Actions&& ... actions) {
    return Argument<T, std::decay_t<Actions> ...>(std::forward<Actions>(actions) ...);
}

template <class T, class ... Actions>
struct ArgumentsNumber<Argument<T, Actions ...>> : ArgumentsNumber<Selector<Actions ...>> {};

template <class T, class ... Actions>
struct ReturnType<Argument<T, Actions ...>> : ReturnType<Selector<Actions ...>> {};

} // namespace router
