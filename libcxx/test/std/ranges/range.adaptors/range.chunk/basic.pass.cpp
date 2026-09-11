// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20
// UNSUPPORTED: no-ranges

#include <__ranges/chunk_view.h>
#include <array>
#include <cassert>
#include <ranges>

int main() {
  std::array<int, 5> input{1, 2, 3, 4, 5};
  auto chunks = input | std::ranges::views::chunk(2);
  assert(chunks.size() == 3);
  int expected = 1;
  for (auto chunk : chunks)
    for (int value : chunk)
      assert(value == expected++);

  std::array<int, 0> empty{};
  assert(std::ranges::views::chunk(empty, 3).size() == 0);
  assert(std::ranges::views::chunk(input, 10).size() == 1);
}
