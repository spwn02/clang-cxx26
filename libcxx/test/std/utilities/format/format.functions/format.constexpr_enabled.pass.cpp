//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: no-localization

// <format>

// P3391R2 (constexpr std::format): a call of the formatting functions is a constant subexpression when every used
// formatter is constexpr-enabled ([format.formatter.spec]): the string and character types (including the debug `?`
// presentation), the integer types and bool, nullptr_t, and the range and tuple formatters built on them.

#include <array>
#include <cassert>
#include <format>
#include <list>
#include <map>
#include <set>
#include <span>
#include <type_traits>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

using namespace std::string_literals;

// Every check is evaluated at compile time and again at run time.
#define CHECK(...)                                                                                                     \
  do {                                                                                                                 \
    static_assert(__VA_ARGS__);                                                                                        \
    assert(__VA_ARGS__);                                                                                               \
  } while (false)

constexpr bool integers() {
  return std::format("{}", 42) == "42" && std::format("{:#x}", 255) == "0xff" && std::format("{:>5}", -7) == "   -7" &&
         std::format("{}", true) == "true" && std::format("{:d}", false) == "0" &&
         std::format("{}", 18446744073709551615ull) == "18446744073709551615";
}

constexpr bool characters() {
  return std::format("{}", 'a') == "a" && std::format("{:?}", 'a') == "'a'" && std::format("{:?}", '\n') == "'\\n'" &&
         std::format("{:?}", '\'') == "'\\''" && std::format(L"{:?}", L'\t') == L"'\\t'";
}

constexpr bool strings() {
  return std::format("{}", "hi") == "hi" && std::format("{:>5}", "hi") == "   hi" &&
         std::format("{:?}", "hi") == "\"hi\"" && std::format("{:?}", "a\tb\"c\\") == "\"a\\tb\\\"c\\\\\"" &&
         std::format("{:?}", "hi"s) == "\"hi\"" && std::format("{:?}", std::string_view("x")) == "\"x\"" &&
         std::format(L"{:?}", L"a\nb") == L"\"a\\nb\"" && std::format("{:?}", "\x01") == "\"\\u{1}\"" &&
         std::format("{:*^8?}", "ab") == "**\"ab\"**";
}

constexpr bool null_pointer() {
  return std::format("{}", nullptr) == "0x0" && std::format("{:>5}", nullptr) == "  0x0" &&
         std::format("{:p}", nullptr) == "0x0" && std::format("{:P}", nullptr) == "0X0" &&
         std::format(L"{}", nullptr) == L"0x0";
}

constexpr bool ranges() {
  int c_array[]{1, 2};
  return std::format("{}", std::vector<int>{1, 2}) == "[1, 2]" && std::format("{}", std::array<int, 2>{1, 2}) == "[1, 2]" &&
         std::format("{}", c_array) == "[1, 2]" && std::format("{}", std::span<int>(c_array)) == "[1, 2]" &&
         std::format("{}", std::vector<std::string>{"a"}) == "[\"a\"]" &&
         std::format("{}", std::vector<std::vector<int>>{{1}, {2, 3}}) == "[[1], [2, 3]]" &&
         std::format("{:n}", std::vector<int>{1, 2}) == "1, 2" && std::format("{::#x}", std::vector<int>{15}) == "[0xf]" &&
         std::format("{}", std::set<int>{1, 2}) == "{1, 2}" && std::format("{}", std::map<int, int>{{1, 2}}) == "{1: 2}" &&
         std::format("{}", std::vector<int>{}) == "[]" && std::format("{:s}", std::vector<char>{'a', 'b'}) == "ab" &&
         std::format("{:?s}", std::vector<char>{'a', '\n'}) == "\"a\\n\"";
}

constexpr bool tuples() {
  return std::format("{}", std::pair{1, 2}) == "(1, 2)" && std::format("{}", std::tuple{1, "a"s}) == "(1, \"a\")" &&
         std::format("{}", std::tuple<>{}) == "()" && std::format("{:n}", std::pair{'a', 2}) == "'a', 2";
}


// A user-defined formatter is used through basic_format_arg::handle; it is constexpr-enabled by declaring its members
// constexpr.
struct Point {
  int x, y;
};

template <>
struct std::formatter<Point> {
  constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
  constexpr auto format(const Point& p, std::format_context& ctx) const {
    return std::format_to(ctx.out(), "({}, {})", p.x, p.y);
  }
};

constexpr bool user_defined() {
  return std::format("{}", Point{1, 2}) == "(1, 2)" && std::format("{}|{}", Point{3, 4}, 5) == "(3, 4)|5";
}

// basic_format_args, basic_format_arg and visit.
constexpr bool arguments() {
  int value  = 5;
  auto store = std::make_format_args(value);
  std::format_args args(store);
  bool is_int = false;
  args.get(0).visit([&](auto v) {
    if constexpr (std::is_same_v<decltype(v), int>)
      is_int = v == 5;
  });
  return is_int && static_cast<bool>(args.get(0)) && !static_cast<bool>(args.get(1)) && !static_cast<bool>(std::basic_format_arg<std::format_context>{});
}

constexpr bool functions() {
  char buffer[8]{};
  auto result = std::format_to(buffer, "{}", 12);
  auto counted = std::format_to_n(buffer, 3, "{}", 12345);
  int value    = 1;
  return result - buffer == 2 && counted.size == 5 && std::formatted_size("{}", 12345) == 5 &&
         std::vformat("{}", std::make_format_args(value)) == "1" &&
         std::format(std::dynamic_format("{}"), 7) == "7";
}

int main(int, char**) {
  CHECK(integers());
  CHECK(characters());
  CHECK(strings());
  CHECK(null_pointer());
  CHECK(ranges());
  CHECK(tuples());
  CHECK(user_defined());
  CHECK(arguments());
  CHECK(functions());
  return 0;
}
