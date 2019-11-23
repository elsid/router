#include <algorithm>
#include <charconv>
#include <cstdio>
#include <iostream>
#include <map>
#include <random>
#include <set>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
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

void roll_dice(State& state) {
    std::printf("dice show %d\n", std::uniform_int_distribution<int>(1, 6)(state.random));
}

void cast(State& state, Wizard wizard, Spell spell) {
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

void learn(State& state, Wizard wizard, Spell spell) {
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

void add_spell(State& state, Spell spell, Mana cost) {
    if (state.spells.find(spell.name) != state.spells.end()) {
        std::printf("spell %.*s already exists\n", int(spell.name.size()), spell.name.data());
        return;
    }
    state.spells.emplace(spell.name, cost.value);
    std::printf("spell %.*s is added\n", int(spell.name.size()), spell.name.data());
}

void add_wizard(State& state, Wizard wizard, Mana mana) {
    if (state.wizards.find(wizard.name) != state.wizards.end()) {
        std::printf("wizard %.*s already exists\n", int(wizard.name.size()), wizard.name.data());
        return;
    }
    state.wizards.emplace(wizard.name, mana.value);
    std::printf("wizard %.*s is added\n", int(wizard.name.size()), wizard.name.data());
}

void channel(State& state, Wizard wizard, Mana mana) {
    const auto it = state.wizards.find(wizard.name);
    if (it == state.wizards.end()) {
        std::printf("wizard %.*s is not found\n", int(wizard.name.size()), wizard.name.data());
        return;
    }
    it->second += mana.value;
    std::printf("wizard %.*s is channeled by %d mana\n", int(wizard.name.size()), wizard.name.data(), mana.value);
}

void wizard_mana(State& state, Wizard wizard) {
    const auto it = state.wizards.find(wizard.name);
    if (it == state.wizards.end()) {
        std::printf("wizard %.*s is not found\n", int(wizard.name.size()), wizard.name.data());
        return;
    }
    std::printf("wizard %.*s has %d mana\n", int(wizard.name.size()), wizard.name.data(), it->second);
}

void spell_cost(State& state, Spell spell) {
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

struct Action {
    virtual ~Action() = default;
    virtual void operator ()(State& state, std::span<std::string> args) const = 0;
};

struct RollDice : Action {
    void operator ()(State& state, std::span<std::string>) const final {
        roll_dice(state);
    }
};

struct Cast : Action {
    void operator ()(State& state, std::span<std::string> args) const final {
        if (args.size() < 2) {
            throw std::runtime_error("Not enough input");
        }
        cast(state, Wizard {args[0]}, Spell {args[1]});
    }
};

struct Learn : Action {
    void operator ()(State& state, std::span<std::string> args) const final {
        if (args.size() < 2) {
            throw std::runtime_error("Not enough input");
        }
        learn(state, Wizard {args[0]}, Spell {args[1]});
    }
};

struct Channel : Action {
    void operator ()(State& state, std::span<std::string> args) const final {
        if (args.size() < 2) {
            throw std::runtime_error("Not enough input");
        }
        channel(state, Wizard {args[0]}, Mana {args[1]});
    }
};

struct WizardMana : Action {
    void operator ()(State& state, std::span<std::string> args) const final {
        if (args.size() < 1) {
            throw std::runtime_error("Not enough input");
        }
        wizard_mana(state, Wizard {args[0]});
    }
};

struct AddSpell : Action {
    void operator ()(State& state, std::span<std::string> args) const final {
        if (args.size() < 2) {
            throw std::runtime_error("Not enough input");
        }
        add_spell(state, Spell {args[0]}, Mana {args[1]});
    }
};

struct AddWizard : Action {
    void operator ()(State& state, std::span<std::string> args) const final {
        if (args.size() < 2) {
            throw std::runtime_error("Not enough input");
        }
        add_wizard(state, Wizard {args[0]}, Mana {args[1]});
    }
};

struct SpellCost : Action {
    void operator ()(State& state, std::span<std::string> args) const final {
        if (args.size() < 1) {
            throw std::runtime_error("Not enough input");
        }
        spell_cost(state, Spell {args[0]});
    }
};

struct Selector : Action {
    const std::map<std::string_view, const Action*> actions;

    explicit Selector(const std::map<std::string_view, const Action*> actions)
        : actions(std::move(actions)) {}

    void operator ()(State& state, std::span<std::string> input) const final {
        if (input.size() < 1) {
            throw std::runtime_error("Not enough input");
        }
        const auto action = actions.find(input[0]);
        if (action != actions.cend()) {
            return (*action->second)(state, std::span(input.data() + 1, input.size() - 1));
        }
        throw std::runtime_error("Invalid action: " + std::string(input[0]));
    }
};

} // namespace

int main() {
    const RollDice roll_dice;
    const AddWizard add_wizard;
    const Cast cast;
    const Learn learn;
    const Channel channel;
    const WizardMana wizard_mana;
    const Selector wizards({
        {"add", &add_wizard},
        {"cast", &cast},
        {"learn", &learn},
        {"channel", &channel},
        {"mana", &wizard_mana},
    });
    const AddSpell add_spell;
    const SpellCost spell_cost;
    const Selector spells({
        {"add", &add_spell},
        {"cost", &spell_cost},
    });
    const Selector dispatch({
        {"roll_dice", &roll_dice},
        {"spells", &spells},
        {"wizards", &wizards},
    });
    State state;
    for (std::string line; std::getline(std::cin, line);) {
        std::printf("\"%s\" ", line.c_str());
        std::vector<std::string> input;
        std::istringstream stream(std::move(line));
        for (std::string arg; stream >> arg;) {
            input.emplace_back(std::move(arg));
        }
        try {
            dispatch(state, input);
        } catch (const std::exception& e) {
            std::printf("failed: %s\n", e.what());
        }
    }
}
