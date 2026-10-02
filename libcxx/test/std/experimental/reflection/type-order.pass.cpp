//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

#include <meta>
#include <compare>
#include <type_traits>

struct Inc;
using Alias = int;
int object;
namespace ns {}
template <class> struct Box {};

static_assert(std::is_same_v<decltype(std::meta::type_order(^^int, ^^char)), std::strong_ordering>);
static_assert(std::meta::type_order(^^int, ^^char) == std::type_order_v<int, char>);
static_assert(std::meta::type_order(^^Inc, ^^int) == std::type_order_v<Inc, int>);
static_assert(std::meta::type_order(^^Alias, ^^int) == std::strong_ordering::equal);
static_assert(std::meta::type_order(^^const int, ^^int) != 0);
static_assert(std::meta::type_order(^^int&, ^^int&&) != 0);
static_assert(std::meta::type_order(^^void, ^^void) == 0);

consteval bool rejects(std::meta::info r, std::meta::info s) {
  try {
    (void)std::meta::type_order(r, s);
  } catch (const std::meta::exception&) {
    return true;
  }
  return false;
}
static_assert(rejects(^^object, ^^int));
static_assert(rejects(^^int, ^^object));
static_assert(rejects(^^ns, ^^int));
static_assert(rejects(^^Box, ^^int));
static_assert(rejects(std::meta::info{}, ^^int));
static_assert(rejects(std::meta::reflect_constant(42), ^^int));

int main(int, char**) { return 0; }
