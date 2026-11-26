// RUN: clang-tidy -checks='-*,modernize-*,readability-*,performance-*' %s -- -std=c++29 2>&1 | FileCheck --allow-empty %s
// RUN: cp %s %t.cpp
// RUN: clang-tidy -checks='-*,modernize-*,readability-*,performance-*' -fix %t.cpp -- -std=c++29
// RUN: clang++ -std=c++29 -fsyntax-only %t.cpp
// CHECK-NOT: warning:
// CHECK-NOT: Assertion
// CHECK-NOT: Stack dump
// NOLINTBEGIN(readability-identifier-naming)

struct Pair { int first; int second; };
struct Empty {};
struct Triple { int first; int second; int third; };
struct Nested { Pair pair; int tail; };

namespace std {
template <class T> struct alternative_traits;
struct alternative_info {
  decltype(^^int) info;
  bool empty = false;
  consteval alternative_info(decltype(^^int) value, bool empty = false)
      : info(value), empty(empty) {}
};
}
struct Choice { unsigned state; int integer; long wide; };
template <> struct std::alternative_traits<Choice> {
  static constexpr std::alternative_info alternatives[] = {^^int, ^^long}; // NOLINT(modernize-avoid-c-arrays)
  static constexpr bool has_residual_states = false;
  static constexpr __SIZE_TYPE__ index(const Choice &value) noexcept { // NOLINT(modernize-use-trailing-return-type)
    return value.state;
  }
  template <__SIZE_TYPE__ I, class Self>
  static constexpr decltype(auto) get(Self &&value) { // NOLINT(modernize-use-trailing-return-type)
    if constexpr (I == 0) {
      return (static_cast<Self &&>(value).integer);
    } else {
      return (static_cast<Self &&>(value).wide);
    }
  }
};

constexpr int empty_decomposition(Empty value) { // NOLINT(modernize-use-trailing-return-type)
  return match(value) { case [] => 42; }; // NOLINT(readability-magic-numbers)
}
static_assert(empty_decomposition({}) == 42); // NOLINT(readability-magic-numbers)

constexpr int sum(Pair value) { // NOLINT(modernize-use-trailing-return-type)
  return match (value) {
    case [0, int value] => value;
    case [int left, int right] => left + right;
  };
}
static_assert(sum({0, 5}) == 5); // NOLINT(modernize-use-designated-initializers,readability-magic-numbers)
static_assert(sum({2, 3}) == 5); // NOLINT(modernize-use-designated-initializers,readability-magic-numbers)

constexpr int nested_or(Nested value) { // NOLINT(modernize-use-trailing-return-type)
  return match(value) {
    case [[0 or 1, int left], int right] => left + right;
    case _ => -1;
  };
}
static_assert(nested_or({{1, 3}, 4}) == 7); // NOLINT(modernize-use-designated-initializers,readability-magic-numbers)

constexpr int type_pattern(int value) { // NOLINT(modernize-use-trailing-return-type)
  return match(value) { case int bound => bound; };
}
static_assert(type_pattern(9) == 9); // NOLINT(readability-magic-numbers)

constexpr int braced_pattern(Choice value) { // NOLINT(modernize-use-trailing-return-type)
  return match(value) {
    case { int bound } => bound;
    case { long bound } => static_cast<int>(bound);
  };
}
static_assert(braced_pattern({0, 1, 0}) == 1); // NOLINT(modernize-use-designated-initializers,readability-magic-numbers)
static_assert(braced_pattern({1, 0, 2}) == 2); // NOLINT(modernize-use-designated-initializers,readability-magic-numbers)

constexpr int condition(Pair value) { // NOLINT(modernize-use-trailing-return-type)
  if (case [int first, auto ...rest] = value)
    return first + (... + rest);
  return 0;
}

constexpr int pack_shapes(Triple value) { // NOLINT(modernize-use-trailing-return-type)
  return match(value) {
    case [auto&& first, ..., auto&& last] => first + last;
  };
}
static_assert(pack_shapes({2, 4, 8}) == 10); // NOLINT(modernize-use-designated-initializers,readability-magic-numbers)

constexpr bool guarded(Pair value) { // NOLINT(modernize-use-trailing-return-type)
  return match(value, case [int bound, 0] if (bound > 0));
}

int with_do(bool choose) { // NOLINT(modernize-use-trailing-return-type)
  int value = do {
    if (choose) do_return 1; // NOLINT(readability-braces-around-statements)
    do_return 2;
  };
  return value;
}

struct Counter {
  int value = 0;
  Counter& operator++() { ++value; return *this; } // NOLINT(modernize-use-trailing-return-type)
  Counter operator++(int) = default; // NOLINT(modernize-use-trailing-return-type)
  Counter& operator--() { --value; return *this; } // NOLINT(modernize-use-trailing-return-type)
  Counter operator--(int) = default; // NOLINT(modernize-use-trailing-return-type)
};

namespace std { using noreturn_t = decltype(throw 0); }
std::noreturn_t stop_now(); // NOLINT(modernize-use-trailing-return-type)
int diverging_conditional(bool stop) { return stop ? stop_now() : 7; } // NOLINT(modernize-use-trailing-return-type,readability-magic-numbers)


// NOLINTEND(readability-identifier-naming)
