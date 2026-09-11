// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20
// UNSUPPORTED: no-ranges

#include <__ranges/slide_view.h>
#include <array>
#include <cassert>
#include <ranges>

int main() {
  std::array<int, 5> input{1, 2, 3, 4, 5};
  auto windows = input | std::ranges::views::slide(3);
  assert(windows.size() == 3);
  int first = 1;
  for (auto window : windows) {
    assert(*window.begin() == first);
    assert(*std::ranges::next(window.begin(), 2) == first + 2);
    ++first;
  }

  std::array<int, 2> short_input{1, 2};
  assert(std::ranges::views::slide(short_input, 3).size() == 0);
}
