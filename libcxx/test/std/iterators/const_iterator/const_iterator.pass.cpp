// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

#include <cassert>
#include <memory>
#include <ranges>
#include <type_traits>
#include <vector>

template <class T>
concept assignable_through = requires(T t) { *t = 42; };

struct Base {};
struct Derived : Base {};

// P2836R1: basic_const_iterator<I> is convertible from basic_const_iterator<I2>
// whenever I2 is convertible to I -- not just from a raw I2.
constexpr bool test_p2836r1_conversion() {
  Derived d;
  Derived* p = &d;
  std::basic_const_iterator<Derived*> derived_iter(p);
  std::basic_const_iterator<Base*> base_iter = derived_iter;
  return static_cast<const void*>(&*base_iter) == static_cast<const void*>(&d);
}
static_assert(std::convertible_to<std::basic_const_iterator<Derived*>, std::basic_const_iterator<Base*>>);
static_assert(!std::convertible_to<std::basic_const_iterator<Base*>, std::basic_const_iterator<Derived*>>);

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
  static_assert(test_p2836r1_conversion());
  assert(test_p2836r1_conversion());

  std::vector<int> values{1, 2, 3};
  auto transformed = values | std::views::transform([](int& value) -> int& { return value; });
  auto first = std::ranges::cbegin(transformed);
  static_assert(!assignable_through<decltype(first)>);
  static_assert(std::same_as<decltype(std::ranges::cend(transformed)),
                             std::basic_const_iterator<decltype(std::ranges::end(transformed))>>);
  static_assert(!assignable_through<decltype(std::views::as_const(values).begin())>);
}
