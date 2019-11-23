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

void dispatch(State& state, std::span<std::string> input) {
    if (input.empty()) {
        throw std::runtime_error("Not enough input");
    }
    if (input.size() == 1) {
        if (input[0] == "roll_dice") {
            return roll_dice(state);
        }
    }
    if (input.size() == 3) {
        if (input[0] == "spells") {
            if (input[1] == "cost") {
                return spell_cost(state, Spell {input[2]});
            }
        }
        if (input[0] == "wizards") {
            if (input[1] == "mana") {
                return wizard_mana(state, Wizard {input[2]});
            }
        }
    }
    if (input.size() == 4) {
        if (input[0] == "spells") {
            if (input[1] == "add") {
                return add_spell(state, Spell {input[2]}, Mana {input[3]});
            }
        }
        if (input[0] == "wizards") {
            if (input[1] == "add") {
                return add_wizard(state, Wizard {input[2]}, Mana {input[3]});
            }
            if (input[1] == "learn") {
                return learn(state, Wizard {input[2]}, Spell {input[3]});
            }
            if (input[1] == "cast") {
                return cast(state, Wizard {input[2]}, Spell {input[3]});
            }
            if (input[1] == "channel") {
                return channel(state, Wizard {input[2]}, Mana {input[3]});
            }
        }
    }
    throw std::runtime_error("Invalid input");
}

} // namespace

int main() {
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
