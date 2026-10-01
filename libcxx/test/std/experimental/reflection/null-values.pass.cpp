//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

#include <meta>
#include <variant>
#include <cassert>

std::meta::info null;
auto v = std::variant<std::meta::info, int>(42);
struct C { std::meta::info const* p; };
auto c = C{.p = nullptr};
consteval int plus1(int x) { return x + 1; }
[[maybe_unused]] constexpr auto b = plus1;
template <auto F> struct Function {};
auto function_argument = Function<plus1>();

void f() {
  std::meta::info i;
  (void)i;
}
int main(int, char**) {
  f();
  assert(null == std::meta::info{});
  assert(v.index() == 1);
  assert(std::get<int>(v) == 42);
  assert(c.p == nullptr);
  return 0;
}
