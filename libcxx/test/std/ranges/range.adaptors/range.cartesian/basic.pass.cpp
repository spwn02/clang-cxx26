// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

#include <array>
#include <cassert>
#include <concepts>
#include <ranges>
#include <tuple>
#include <vector>

int main(int, char**) {
  int a[] = {1, 2};
  char b[] = {'a', 'b', 'c'};
  auto product = std::views::cartesian_product(a, b);
  static_assert(std::ranges::sized_range<decltype(product)>);
  static_assert(std::random_access_iterator<decltype(product.begin())>);
  static_assert(std::same_as<std::iter_difference_t<decltype(product.begin())>, std::ptrdiff_t>);
  assert(product.size() == 6);

  std::vector<std::tuple<int, char>> got;
  for (auto [x, y] : product)
    got.emplace_back(x, y);
  assert((got == std::vector<std::tuple<int, char>>{{1, 'a'}, {1, 'b'}, {1, 'c'}, {2, 'a'}, {2, 'b'}, {2, 'c'}}));
  assert(std::get<0>(product.begin()[4]) == 2 && std::get<1>(product.begin()[4]) == 'b');

  auto one = std::views::cartesian_product(a);
  assert(one.size() == 2);
  assert(std::get<0>(*one.begin()) == 1);

  auto empty = std::views::cartesian_product(a, std::array<int, 0>{});
  assert(empty.begin() == empty.end());
  assert(empty.size() == 0);

  auto unit = std::views::cartesian_product();
  assert(unit.size() == 1);
  assert(unit.begin() != unit.end());
}
