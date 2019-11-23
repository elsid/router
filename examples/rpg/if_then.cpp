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

void eat(Food food) {
    std::printf("%.*s is eaten\n", int(food.name.size()), food.name.data());
}

void cast(Spell spell) {
    std::printf("%.*s is casted\n", int(spell.name.size()), spell.name.data());
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
    throw std::runtime_error("Invalid action: " + std::string(input[0]));
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
