#include <algorithm>
#include <cstdio>
#include <iostream>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <map>
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

struct Action {
    virtual ~Action() = default;
    virtual void operator ()(std::string_view object) const = 0;
};

struct Eat : Action {
    void operator ()(std::string_view food) const final {
        eat(Food {food});
    }
};

struct Cast : Action {
    void operator ()(std::string_view spell) const final {
        cast(Spell {spell});
    }
};

struct Selector {
    const std::map<std::string_view, const Action*> actions;

    void operator ()(std::span<std::string> input) const {
        if (input.size() < 2) {
            throw std::runtime_error("Not enough input");
        }
        const auto action = actions.find(input[0]);
        if (action != actions.cend()) {
            return (*action->second)(input[1]);
        }
        throw std::runtime_error("Invalid action: " + std::string(input[0]));
    }
};

} // namespace

int main() {
    const Eat eat;
    const Cast cast;
    const Selector dispatch {{
        {"eat", &eat},
        {"cast", &cast},
    }};
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
