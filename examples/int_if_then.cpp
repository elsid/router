#include <iostream>
#include <iterator>
#include <ranges>
#include <span>
#include <vector>

inline auto consume(std::ranges::input_range auto input) {
    return std::ranges::subrange(std::next(std::begin(input)), std::end(input));
}

inline int increment(int value) {
    return value + 1;
}

inline int decrement(int value) {
    return value - 1;
}

int dispatch(std::span<int> input) {
  if (input.empty()) {
      std::abort();
  }
  if (input[0] == 3457384) {
      input = consume(input);
      if (input.empty()) {
          std::abort();
      }
      const auto arg = input[0];
      input = consume(input);
      if (!input.empty()) {
          std::abort();
      }
      return increment(arg);
  }
  if (input[0] == 5438990) {
      input = consume(input);
      if (input.empty()) {
          std::abort();
      }
      const auto arg = input[0];
      input = consume(input);
      if (!input.empty()) {
          std::abort();
      }
      return decrement(arg);
  }
  std::abort();
}

int main() {
    std::vector a {3457384, 42};
    std::vector b {5438990, 13};
    std::cout << dispatch(a) << std::endl;
    std::cout << dispatch(b) << std::endl;
    return 0;
}
