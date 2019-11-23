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
#include <vector>

namespace model {

struct Spell {
    std::string_view name;

    Spell(std::string_view name) : name(name) {}
};

struct Wizard {
    std::string_view name;

    Wizard(std::string_view name) : name(name) {}
};

struct Mana {
    unsigned value = 0;

    Mana(std::string_view raw) {
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

State state;

void roll_dice() {
    std::printf("dice show %d\n", std::uniform_int_distribution<int>(1, 6)(state.random));
}

void cast(Wizard wizard, Spell spell) {
    const auto wizard_it = state.wizards.find(wizard.name);
    if (wizard_it == state.wizards.end()) {
        std::printf("wizard %.*s is not found\n", int(wizard.name.size()), wizard.name.data());
        return;
    }
    const auto spell_it = state.spells.find(spell.name);
    if (spell_it == state.spells.end()) {
        std::printf("spell %.*s is not found\n", int(spell.name.size()), spell.name.data());
        return;
    }
    const auto it = state.known_spells.find(wizard.name);
    if (it == state.known_spells.end() || it->second.find(spell.name) == it->second.end()) {
        std::printf("wizard %.*s doesn't know spell %.*s\n", int(wizard.name.size()), wizard.name.data(), int(spell.name.size()), spell.name.data());
        return;
    }
    if (wizard_it->second < spell_it->second) {
        std::printf("wizard %.*s doesn't have enough mana to cast %.*s\n", int(wizard.name.size()), wizard.name.data(), int(spell.name.size()), spell.name.data());
        return;
    }
    wizard_it->second -= spell_it->second;
    std::printf("spell %.*s is casted by wizard %.*s\n", int(spell.name.size()), spell.name.data(), int(wizard.name.size()), wizard.name.data());
}

void learn(Wizard wizard, Spell spell) {
    const auto wizard_it = state.wizards.find(wizard.name);
    if (state.wizards.find(wizard.name) == state.wizards.end()) {
        std::printf("wizard %.*s is not found\n", int(wizard.name.size()), wizard.name.data());
        return;
    }
    const auto spell_it = state.spells.find(spell.name);
    if (spell_it == state.spells.end()) {
        std::printf("spell %.*s is not found\n", int(spell.name.size()), spell.name.data());
        return;
    }
    auto it = state.known_spells.find(wizard.name);
    if (it == state.known_spells.end()) {
        it = state.known_spells.emplace(wizard_it->first, std::set<std::string_view>()).first;
    }
    if (it->second.insert(spell_it->first).second) {
        std::printf("wizard %.*s has learned spell %.*s\n", int(wizard.name.size()), wizard.name.data(), int(spell.name.size()), spell.name.data());
    }
}

void add_spell(Spell spell, Mana cost) {
    if (state.spells.find(spell.name) != state.spells.end()) {
        std::printf("spell %.*s already exists\n", int(spell.name.size()), spell.name.data());
        return;
    }
    state.spells.emplace(spell.name, cost.value);
    std::printf("spell %.*s is added\n", int(spell.name.size()), spell.name.data());
}

void add_wizard(Wizard wizard, Mana mana) {
    if (state.wizards.find(wizard.name) != state.wizards.end()) {
        std::printf("wizard %.*s already exists\n", int(wizard.name.size()), wizard.name.data());
        return;
    }
    state.wizards.emplace(wizard.name, mana.value);
    std::printf("wizard %.*s is added\n", int(wizard.name.size()), wizard.name.data());
}

void channel(Wizard wizard, Mana mana) {
    const auto it = state.wizards.find(wizard.name);
    if (it == state.wizards.end()) {
        std::printf("wizard %.*s is not found\n", int(wizard.name.size()), wizard.name.data());
        return;
    }
    it->second += mana.value;
    std::printf("wizard %.*s is channeled by %d mana\n", int(wizard.name.size()), wizard.name.data(), mana.value);
}

void wizard_mana(Wizard wizard) {
    const auto it = state.wizards.find(wizard.name);
    if (it == state.wizards.end()) {
        std::printf("wizard %.*s is not found\n", int(wizard.name.size()), wizard.name.data());
        return;
    }
    std::printf("wizard %.*s has %d mana\n", int(wizard.name.size()), wizard.name.data(), it->second);
}

void spell_cost(Spell spell) {
    const auto it = state.spells.find(spell.name);
    if (it == state.spells.end()) {
        std::printf("spell %.*s is not found\n", int(spell.name.size()), spell.name.data());
        return;
    }
    std::printf("spell %.*s costs %d mana\n", int(spell.name.size()), spell.name.data(), it->second);
}

} // namespace model

namespace {

using namespace model;

struct RollDice {
    static constexpr std::string_view name {"roll_dice"};

    void operator ()() const {
        roll_dice();
    }
};

struct Cast {
    static constexpr std::string_view name {"cast"};

    void operator ()(std::string_view wizard, std::string_view spell) const {
        cast(Wizard {wizard}, Spell {spell});
    }
};

struct Learn {
    static constexpr std::string_view name {"learn"};

    void operator ()(std::string_view wizard, std::string_view spell) const {
        learn(Wizard {wizard}, Spell {spell});
    }
};

struct AddSpell {
    static constexpr std::string_view name {"add"};

    void operator ()(std::string_view spell, std::string_view cost) const {
        add_spell(Spell {spell}, Mana(cost));
    }
};

struct AddWizard {
    static constexpr std::string_view name {"add"};

    void operator ()(std::string_view wizard, std::string_view mana) const {
        add_wizard(Wizard {wizard}, Mana(mana));
    }
};

struct Channel {
    static constexpr std::string_view name {"channel"};

    void operator ()(std::string_view wizard, std::string_view mana) const {
        channel(Wizard {wizard}, Mana(mana));
    }
};

struct WizardMana {
    static constexpr std::string_view name {"mana"};

    void operator ()(std::string_view wizard) const {
        wizard_mana(Wizard {wizard});
    }
};

struct SpellCost {
    static constexpr std::string_view name {"cost"};

    void operator ()(std::string_view spell) const {
        spell_cost(Spell {spell});
    }
};

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

template <class F>
constexpr std::size_t arguments_number_v = ArgumentsNumber<F>::value;

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

template <class Tag, class ... Actions>
struct Selector {
    static constexpr typename Tag::value_type name = Tag::value;

    const std::tuple<Actions ...> actions;

    template <class ... Ts>
    constexpr explicit Selector(Tag, Ts&& ... actions) : actions(std::forward<Ts>(actions) ...) {}

    template <class ... Args>
    void operator ()(std::ranges::input_range auto input, Args&& ... args) const {
        if (std::empty(input)) {
            throw std::runtime_error("Not enough input");
        }
        return find_action(*std::begin(input), [&] (const auto& action) {
            return invoke(action, consume(input), std::forward<Args>(args) ...);
        });
    }

    template <std::size_t i = 0, class V, class F>
    void find_action(const V& action_name, F&& f) const {
        if constexpr (i >= std::tuple_size_v<decltype(actions)>) {
            throw std::runtime_error("Invalid action: " + std::string(action_name));
        } else {
            if (std::get<i>(actions).name == action_name) {
                return f(std::get<i>(actions));
            }
            find_action<i + 1>(action_name, std::forward<F>(f));
        }
    }
};

template <class Tag, class ... Actions>
Selector(Tag, Actions&& ...) -> Selector<Tag, std::decay_t<Actions> ...>;

struct Tag {
    using value_type = std::string_view;
};

constexpr struct SpellsTag : Tag {
    static constexpr value_type value {"spells"};
} spells_tag;

constexpr struct WizardsTag : Tag {
    static constexpr value_type value {"wizards"};
} wizards_tag;

constexpr struct EmptyTag : Tag {
    [[maybe_unused]]
    static constexpr value_type value {};
} empty_tag;

} // namespace

int main() {
    constexpr Selector dispatch(
        empty_tag,
        RollDice {},
        Selector(spells_tag, AddSpell {}, SpellCost {}),
        Selector(wizards_tag, AddWizard {}, Cast {}, Learn {}, Channel {}, WizardMana {})
    );
    for (std::string line; std::getline(std::cin, line);) {
        std::printf("\"%s\" ", line.c_str());
        std::vector<std::string> input;
        std::istringstream stream(std::move(line));
        for (std::string arg; stream >> arg;) {
            input.emplace_back(std::move(arg));
        }
        try {
            dispatch(input);
        } catch (const std::exception& e) {
            std::printf("failed: %s\n", e.what());
        }
    }
}
