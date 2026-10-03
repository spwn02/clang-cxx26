// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

// P4101R1 value-based consteval-only model: the accepted forms from the
// [expr.const.const] example, and the CWG3150 incomplete-class example (#196).

#include <meta>
#include <variant>

consteval int plus1(int x) { return x + 1; }
template <auto V> struct C {};

[[maybe_unused]] constexpr auto b = plus1;                         // OK
[[maybe_unused]] auto c = C<plus1>();                              // OK
[[maybe_unused]] auto e = C<^^char>();                             // OK

std::meta::info null;                             // null reflection is not consteval-only
auto v = std::variant<std::meta::info, int>(42);  // holds an int
struct HoldsPointer { std::meta::info const* p; };
auto holds = HoldsPointer{.p = nullptr};
void local_null() { std::meta::info i; (void)i; }

// CWG3150: an incomplete class later given a consteval-only member.
struct S;
void f(S*);
struct S { std::meta::info x; };
void f(S*) {}

int main(int, char**) {
  (void)null; (void)v; (void)holds;
  local_null();
  S* p = nullptr;
  f(p);
  return 0;
}
