#include <cstdio>
#include <iostream>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace model {

struct Food {
    std::string_view name;
};

struct Spell {
    std::string_view name;
};

struct Potion {
    std::string_view name;
};

struct Staff {
    std::string_view name;
};

struct Robe {
    std::string_view name;
};

struct Scroll {
    std::string_view name;
};

void eat(Food food) {
    std::printf("%.*s is eaten\n", int(food.name.size()), food.name.data());
}

void cast(Spell spell) {
    std::printf("%.*s is casted\n", int(spell.name.size()), spell.name.data());
}

void drink(Potion beverage) {
    std::printf("%.*s is drank\n", int(beverage.name.size()), beverage.name.data());
}

void wear(Robe robe) {
    std::printf("%.*s is weared\n", int(robe.name.size()), robe.name.data());
}

void equip(Staff staff) {
    std::printf("%.*s is equiped\n", int(staff.name.size()), staff.name.data());
}

void enchant(Staff staff) {
    std::printf("%.*s is enchanted\n", int(staff.name.size()), staff.name.data());
}

void read(Scroll scroll) {
    std::printf("%.*s is read\n", int(scroll.name.size()), scroll.name.data());
}

} // namespace model

namespace {

using namespace model;

void dispatch(std::span<std::string> input) {
    if (input.size() < 2) {
        throw std::runtime_error("Not enough input");
    }
    if (input[0] == "eat") {
        return eat(Food {input[1]});
    }
    if (input[0] == "cast") {
        return cast(Spell {input[1]});
    }
    if (input[0] == "drink") {
        return drink(Potion {input[1]});
    }
    if (input[0] == "equip") {
        return equip(Staff {input[1]});
    }
    if (input[0] == "wear") {
        return wear(Robe {input[1]});
    }
    if (input[0] == "enchant") {
        return enchant(Staff {input[1]});
    }
    if (input[0] == "read") {
        return read(Scroll {input[1]});
    }
    throw std::invalid_argument("Invalid action: " + std::string(input[0]));
}

} // namespace

int main() {
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
