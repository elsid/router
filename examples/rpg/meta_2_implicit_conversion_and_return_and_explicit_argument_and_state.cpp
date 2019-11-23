#include <algorithm>
#include <charconv>
#include <cstdio>
#include <iostream>
#include <iterator>
#include <map>
#include <random>
#include <ranges>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <variant>
#include <vector>

namespace model {

struct Spell {
    std::string_view name;

    Spell(const std::string& name) : name(name) {}
};

struct Wizard {
    std::string_view name;

    Wizard(const std::string& name) : name(name) {}
};

struct Mana {
    unsigned value = 0;

    Mana(const std::string& raw) {
        if (auto [_, ec] = std::from_chars(raw.data(), raw.data() + raw.size(), value); ec != std::errc()) {
            throw std::system_error(std::make_error_code(ec));
        }
    }
};

struct State {
    std::minstd_rand0 random;
    std::map<std::string, int, std::less<>> spells;
    std::map<std::string, int, std::less<>> wizards;
    std::map<std::string_view, std::set<std::string_view>> known_spells;
};

struct DiceResult {
    int value;
};

DiceResult roll_dice(State& state) {
    return DiceResult {std::uniform_int_distribution<int>(1, 6)(state.random)};
}

std::error_code cast(State& state, Wizard wizard, Spell spell) {
    const auto wizard_it = state.wizards.find(wizard.name);
    if (wizard_it == state.wizards.end()) {
        return std::make_error_code(std::errc::invalid_argument);
    }
    const auto spell_it = state.spells.find(spell.name);
    if (spell_it == state.spells.end()) {
        return std::make_error_code(std::errc::invalid_argument);
    }
    const auto it = state.known_spells.find(wizard.name);
    if (it == state.known_spells.end() || it->second.find(spell.name) == it->second.end()) {
        return std::make_error_code(std::errc::invalid_argument);
    }
    if (wizard_it->second < spell_it->second) {
        return std::make_error_code(std::errc::invalid_argument);
    }
    wizard_it->second -= spell_it->second;
    std::printf("spell %.*s is casted by wizard %.*s\n", int(spell.name.size()), spell.name.data(), int(wizard.name.size()), wizard.name.data());
    return std::error_code();
}

std::error_code learn(State& state, Wizard wizard, Spell spell) {
    const auto wizard_it = state.wizards.find(wizard.name);
    if (state.wizards.find(wizard.name) == state.wizards.end()) {
        return std::make_error_code(std::errc::invalid_argument);
    }
    const auto spell_it = state.spells.find(spell.name);
    if (spell_it == state.spells.end()) {
        return std::make_error_code(std::errc::invalid_argument);
    }
    auto it = state.known_spells.find(wizard.name);
    if (it == state.known_spells.end()) {
        it = state.known_spells.emplace(wizard_it->first, std::set<std::string_view>()).first;
    }
    if (it->second.insert(spell_it->first).second) {
        std::printf("wizard %.*s has learned spell %.*s\n", int(wizard.name.size()), wizard.name.data(), int(spell.name.size()), spell.name.data());
    }
    return std::error_code();
}

std::error_code add_spell(State& state, Spell spell, Mana cost) {
    if (state.spells.find(spell.name) != state.spells.end()) {
        return std::make_error_code(std::errc::invalid_argument);
    }
    state.spells.emplace(spell.name, cost.value);
    std::printf("spell %.*s is added\n", int(spell.name.size()), spell.name.data());
    return std::error_code();
}

std::error_code add_wizard(State& state, Wizard wizard, Mana mana) {
    if (state.wizards.find(wizard.name) != state.wizards.end()) {
        return std::make_error_code(std::errc::invalid_argument);
    }
    state.wizards.emplace(wizard.name, mana.value);
    std::printf("wizard %.*s is added\n", int(wizard.name.size()), wizard.name.data());
    return std::error_code();
}

std::error_code channel(State& state, Wizard wizard, Mana mana) {
    const auto it = state.wizards.find(wizard.name);
    if (it == state.wizards.end()) {
        return std::make_error_code(std::errc::invalid_argument);
    }
    it->second += mana.value;
    std::printf("wizard %.*s is channeled by %d mana\n", int(wizard.name.size()), wizard.name.data(), mana.value);
    return std::error_code();
}

std::error_code wizard_mana(State& state, Wizard wizard) {
    const auto it = state.wizards.find(wizard.name);
    if (it == state.wizards.end()) {
        return std::make_error_code(std::errc::invalid_argument);
    }
    std::printf("wizard %.*s has %d mana\n", int(wizard.name.size()), wizard.name.data(), it->second);
    return std::error_code();
}

std::error_code spell_cost(State& state, Spell spell) {
    const auto it = state.spells.find(spell.name);
    if (it == state.spells.end()) {
        return std::make_error_code(std::errc::invalid_argument);
    }
    std::printf("spell %.*s costs %d mana\n", int(spell.name.size()), spell.name.data(), it->second);
    return std::error_code();
}

} // namespace model

namespace {

using namespace model;

struct Cast {
    static constexpr std::string_view name {"cast"};

    std::error_code operator ()(State& state, Wizard wizard, Spell spell) const {
        return cast(state, wizard, spell);
    }
};

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

auto consume(std::ranges::input_range auto input) {
    return std::ranges::subrange(std::next(std::begin(input)), std::end(input));
}

template <class Action, std::ranges::input_range Range, class ... Args>
auto invoke(const Action& action, Range input, Args&& ... args) {
    if constexpr (std::is_invocable_v<Action, Range, Args&& ...>) {
        return action(input, std::forward<Args>(args) ...);
    } else if constexpr (sizeof ... (Args) >= arguments_number_v<Action>) {
        if (!std::empty(input)) {
            throw std::runtime_error("Too many arguments");
        }
        return action(std::forward<Args>(args) ...);
    } else {
        if (std::empty(input)) {
            throw std::runtime_error("Not enough input");
        }
        return invoke(action, consume(input),
                      std::forward<Args>(args) ..., *std::begin(input));
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
            throw std::runtime_error("Not enough input");
        }
        return find_action(input, [&] (std::ranges::input_range auto input, const auto& action) {
            return invoke(action, input, std::forward<Args>(args) ...);
        });
    }

    template <std::size_t i = 0, std::ranges::input_range Range, class F>
    return_type find_action(Range input, F&& f) const {
        if constexpr (i >= std::tuple_size_v<decltype(actions)>) {
            throw std::runtime_error("Invalid action: " + std::string(*std::begin(input)));
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

template <class ... Actions>
struct ReturnType<Selector<Actions ...>> {
    using type = typename Selector<Actions ...>::return_type;
};

template <class T, class ... Actions>
struct Argument {
    Selector<Actions ...> selector;

    template <class ... F>
    constexpr explicit Argument(F&& ... f) : selector(std::forward<F>(f) ...) {}

    template <class ... Args>
    auto operator ()(std::ranges::input_range auto input, Args&& ... args) const {
        if (std::empty(input)) {
            throw std::runtime_error("Not enough input");
        }
        return selector(consume(input),
                        std::forward<Args>(args) ..., T {*std::begin(input)});
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

struct Tag {
    using value_type = std::string_view;
};

constexpr struct RollDice : Tag {
    static constexpr std::string_view value {"roll_dice"};
} roll_dice_tag;

constexpr struct LearnTag : Tag {
    static constexpr std::string_view value {"learn"};
} learn_tag;

constexpr struct AddSpellTag : Tag {
    static constexpr std::string_view value {"add"};
} add_spell_tag;

constexpr struct AddWizardTag : Tag {
    static constexpr std::string_view value {"add"};
} add_wizard_tag;

constexpr struct ChannelTag : Tag {
    static constexpr std::string_view value {"channel"};
} channel_tag;

constexpr struct WizardManaTag : Tag {
    static constexpr std::string_view value {"mana"};
} wizard_mana_tag;

constexpr struct SpellCostTag : Tag {
    static constexpr std::string_view value {"cost"};
} spell_cost_tag;

constexpr struct SpellsTag : Tag {
   static constexpr std::string_view value {"spells"};
} spells_tag;

constexpr struct WizardsTag : Tag {
   static constexpr std::string_view value {"wizards"};
} wizards_tag;

struct PrintResult {
    void operator ()(std::monostate) const {}

    void operator ()(DiceResult result) const {
        std::printf("dice show %d\n", result.value);
    }

    void operator ()(std::error_code ec) const {
        if (ec != std::error_code()) {
            const auto message = ec.message();
            std::printf("error: %s\n", message.c_str());
        }
    }
};

} // namespace

int main() {
    constexpr Selector dispatch(
        Action(roll_dice_tag, &roll_dice),
        Action(spells_tag, Selector(
            Action(add_spell_tag, &add_spell),
            argument<Spell>(
                Action(spell_cost_tag, [] (State& state, Spell spell) { return spell_cost(state, spell); })
            )
        )),
        Action(wizards_tag, Selector(
            Action(add_wizard_tag, &add_wizard),
            argument<Wizard>(
                Cast {},
                Action(learn_tag, &learn),
                Action(channel_tag, &channel),
                Action(wizard_mana_tag, &wizard_mana)
            )
        ))
    );
    State state;
    for (std::string line; std::getline(std::cin, line);) {
        std::printf("\"%s\" ", line.c_str());
        std::vector<std::string> input;
        std::istringstream stream(std::move(line));
        for (std::string arg; stream >> arg;) {
            input.emplace_back(std::move(arg));
        }
        try {
            std::visit(PrintResult{}, dispatch(input, state));
        } catch (const std::exception& e) {
            std::printf("failed: %s\n", e.what());
        }
    }
}
