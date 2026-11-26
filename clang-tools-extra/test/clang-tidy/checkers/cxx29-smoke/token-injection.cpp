// RUN: clang-tidy -checks='-*,modernize-*,readability-*,performance-*' %s -- -std=c++29 2>&1 | FileCheck --allow-empty %s
// RUN: cp %s %t.cpp
// RUN: clang-tidy -checks='-*,modernize-*,readability-*,performance-*' -fix %t.cpp -- -std=c++29
// RUN: clang++ -std=c++29 -fsyntax-only %t.cpp
// CHECK-NOT: warning:
// CHECK-NOT: Assertion
// CHECK-NOT: Stack dump
// NOLINTBEGIN(readability-identifier-naming)

namespace std::meta {
using info = decltype(^^::);
template <class... Ts> consteval auto id(Ts const &...) -> int;
using token = decltype(id("x"));
class token_sequence { // NOLINT(readability-identifier-naming)
public:
  constexpr token_sequence() noexcept = default;
  friend consteval auto operator+=(token_sequence &, const token_sequence &)
      -> token_sequence &;
  friend consteval auto operator+(token_sequence, const token_sequence &)
      -> token_sequence;
  friend consteval auto operator==(const token_sequence &,
                                   const token_sequence &) -> bool;
  friend consteval auto operator==(token, const token_sequence &) -> bool;
};
consteval void queue_injection(token_sequence); // NOLINT(readability-identifier-naming)
}

constexpr int factor = 3;
constexpr std::meta::token_sequence expression = ^^{ int injected = \(factor); };

consteval void add_declaration() { // NOLINT(readability-identifier-naming)
  std::meta::queue_injection(^^{ inline constexpr int queued = 42; }); // NOLINT(readability-magic-numbers)
}
consteval { add_declaration(); } // NOLINT(modernize-use-do-expr)
static_assert(queued == 42); // NOLINT(readability-magic-numbers)

struct add_member {
  consteval auto inject_members(std::meta::info) const -> std::meta::token_sequence { // NOLINT(readability-named-parameter)
    return ^^{ int injected_member = 17; }; // NOLINT(readability-magic-numbers)
  }
};
struct [[=add_member{}]] Widget {};
static_assert(Widget{}.injected_member == 17); // NOLINT(readability-magic-numbers)

__macro add_one(int value) { return ^^{ (\(value) + 1) }; } // NOLINT(readability-identifier-naming)
macro forward_value(auto&& value) { return ^^{ \(value) }; } // NOLINT(readability-identifier-naming)
template <class T> macro identity(T&& value) { return ^^{ \(value) }; } // NOLINT(readability-identifier-naming)

auto macro_declarations() -> int { // NOLINT(readability-identifier-naming)
  return add_one!(4) + forward_value!(3) + identity!(2);
}


// NOLINTEND(readability-identifier-naming)
