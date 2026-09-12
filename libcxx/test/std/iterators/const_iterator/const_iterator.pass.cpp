// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

#include <ranges>
#include <type_traits>
#include <vector>

template <class T>
concept assignable_through = requires(T t) { *t = 42; };

constexpr bool test() {
  int values[] = {1, 2, 3};
  auto iterator = std::make_const_iterator(values);
  static_assert(std::is_same_v<decltype(iterator), std::basic_const_iterator<int*>>);
  static_assert(!assignable_through<decltype(iterator)>);
  static_assert(std::is_same_v<decltype(std::make_const_iterator(static_cast<int const*>(nullptr))), int const*>);
  static_assert(std::is_same_v<decltype(std::make_const_sentinel(values + 3)), std::basic_const_iterator<int*>>);
  return *iterator == 1;
}

int main() {
  static_assert(test());

  std::vector<int> values{1, 2, 3};
  auto transformed = values | std::views::transform([](int& value) -> int& { return value; });
  auto first = std::ranges::cbegin(transformed);
  static_assert(!assignable_through<decltype(first)>);
  static_assert(std::same_as<decltype(std::ranges::cend(transformed)),
                             std::basic_const_iterator<decltype(std::ranges::end(transformed))>>);
  static_assert(!assignable_through<decltype(std::views::as_const(values).begin())>);
}
