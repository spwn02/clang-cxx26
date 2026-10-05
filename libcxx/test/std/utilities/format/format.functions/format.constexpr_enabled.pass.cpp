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
#include <stack>
#include <ranges>
#include <queue>
#include <iterator>
#include <deque>
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

#include "test_macros.h"

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
         std::format("{:?}", '\'') == "'\\''";
}

constexpr bool strings() {
  return std::format("{}", "hi") == "hi" && std::format("{:>5}", "hi") == "   hi" &&
         std::format("{:?}", "hi") == "\"hi\"" && std::format("{:?}", "a\tb\"c\\") == "\"a\\tb\\\"c\\\\\"" &&
         std::format("{:?}", "hi"s) == "\"hi\"" && std::format("{:?}", std::string_view("x")) == "\"x\"" &&
         std::format("{:?}", "\x01") == "\"\\u{1}\"" &&
         std::format("{:*^8?}", "ab") == "**\"ab\"**";
}

constexpr bool null_pointer() {
  return std::format("{}", nullptr) == "0x0" && std::format("{:>5}", nullptr) == "  0x0" &&
         std::format("{:p}", nullptr) == "0x0" && std::format("{:P}", nullptr) == "0X0";
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

// Container adaptors, vector<bool>, widening, ill-formed code units, output iterators, dynamic width/precision,
// views, nested ranges and tuples: each case is a constexpr function checked at compile time and at run time.
#define CASE(name, ...)                                                                                                \
  constexpr bool name() { return __VA_ARGS__; }                                                                        \
  static_assert(name(), #name);

CASE(stack_, std::format("{}", std::stack<int>{std::deque<int>{1,2}}) == "[1, 2]")
CASE(queue_, std::format("{}", std::queue<int>{std::deque<int>{1,2}}) == "[1, 2]")
CASE(pqueue_, []{ std::priority_queue<int> q; q.push(1); return std::format("{}", q) == "[1]"; }())
CASE(vecbool_, std::format("{}", std::vector<bool>{true,false}) == "[true, false]")
CASE(vecbool_ref, []{ std::vector<bool> v{true}; return std::format("{}", v[0]) == "true"; }())
CASE(vecbool_cref, []{ const std::vector<bool> v{true}; return std::format("{}", v[0]) == "true"; }())
CASE(ill_utf8, std::format("{:?}", "\xff") == "\"\\x{ff}\"")
CASE(ill_utf8_2, std::format("{:?}", "a\xc3") == "\"a\\x{c3}\"")
CASE(unicode_esc, std::format("{:?}", "\u00e9") == "\"\u00e9\"")
CASE(zero_width, std::format("{:?}", "\u200b") == "\"\\u{200b}\"")
CASE(back_ins, []{ std::string s; std::format_to(std::back_inserter(s), "{}-{}", 1, "a"); return s == "1-a"; }())
CASE(arr_iter, []{ std::array<char,8> a{}; auto r = std::format_to(a.begin(), "{}", 123); return r - a.begin() == 3; }())
CASE(cptr, []{ const char* p = "hi"; return std::format("{}", p) == "hi"; }())
CASE(char_ptr, []{ char b[] = "hi"; char* p = b; return std::format("{}", p) == "hi"; }())
CASE(char_arr, []{ char b[] = "hi"; return std::format("{}", b) == "hi"; }())
CASE(sint_, std::format("{} {} {} {} {}", (signed char)-1, (short)-2, -3, -4l, -5ll) == "-1 -2 -3 -4 -5")
CASE(uint_, std::format("{} {} {} {} {}", (unsigned char)1, (unsigned short)2, 3u, 4ul, 5ull) == "1 2 3 4 5")
CASE(int128, std::format("{}", (__int128)123) == "123")
CASE(uint128, std::format("{}", (unsigned __int128)123) == "123")
CASE(bases, std::format("{:b} {:#o} {:X} {:+d} {:08}", 5, 8, 255, 3, 42) == "101 010 FF +3 00000042")
CASE(dyn_width, std::format("{:{}}", 1, 5) == "    1")
CASE(dyn_both, std::format("{:*<{}}|{:{}.{}}", 'x', 3, "abcdef", 4, 2) == "x**|ab  ")
CASE(indexed, std::format("{1} {0} {1}", 1, 2) == "2 1 2")
CASE(bool_, std::format("{:s} {:d}", true, false) == "true 0")
CASE(char_int, std::format("{:d} {:x}", 'a', 'a') == "97 61")
CASE(str_prec, std::format("{:.2} {:>6.3}", "abcdef", "abcdef") == "ab    abc")
CASE(range_iota, std::format("{}", std::views::iota(0, 3)) == "[0, 1, 2]")
CASE(range_trans, std::format("{}", std::vector<int>{1,2,3} | std::views::transform([](int x){ return x * 2; })) == "[2, 4, 6]")
CASE(range_chars, std::format("{}", std::vector<char>{'a','b'}) == "['a', 'b']")
CASE(range_str, std::format("{:s}", std::string("ab") | std::views::all) == "ab")
CASE(range_list, std::format("{}", std::list<int>{1,2}) == "[1, 2]")
CASE(range_nspec, std::format("{::>3}", std::vector<int>{1,2}) == "[  1,   2]")
CASE(range_ms, std::format("{}", std::multiset<int>{1,1}) == "{1, 1}")
CASE(map_str, std::format("{}", std::map<std::string,int>{{"a",1}}) == "{\"a\": 1}")
CASE(tuple_nested, std::format("{}", std::tuple{1, std::pair{2, 3}}) == "(1, (2, 3))")
CASE(tuple_m, std::format("{:m}", std::tuple{1, 2}) == "1: 2")
CASE(vformat_to, []{ std::string s; int x = 7; std::vformat_to(std::back_inserter(s), "{}", std::make_format_args(x)); return s == "7"; }())

#ifndef TEST_HAS_NO_WIDE_CHARACTERS
CASE(wide_debug_char, std::format(L"{:?}", L'\t') == L"'\\t'")
CASE(wide_debug_string, std::format(L"{:?}", L"a\nb") == L"\"a\\nb\"")
CASE(wide_nullptr, std::format(L"{}", nullptr) == L"0x0")
CASE(widen_char, std::format(L"{}", 'a') == L"a")
CASE(wide_str, std::format(L"{}", L"hi") == L"hi")
CASE(wide_string, std::format(L"{}", L"hi"s) == L"hi")
CASE(wback_ins, []{ std::wstring s; std::format_to(std::back_inserter(s), L"{}", 12); return s == L"12"; }())
CASE(range_w, std::format(L"{}", std::vector<int>{1,2}) == L"[1, 2]")
CASE(pair_w, std::format(L"{}", std::pair{1, L"x"s}) == L"(1, \"x\")")
CASE(fmt_to_n_w, []{ wchar_t b[4]{}; auto r = std::format_to_n(b, 2, L"{}", 123); return r.size == 3; }())
CASE(fsize_w, std::formatted_size(L"{}", 1234) == 4)
CASE(wvformat, []{ int x = 7; return std::vformat(L"{}", std::make_wformat_args(x)) == L"7"; }())
#endif // TEST_HAS_NO_WIDE_CHARACTERS

// std::range_formatter is public: a user formatter built on it is constexpr-enabled as well.
struct Numbers {
  std::vector<int> values;
};

template <>
struct std::formatter<Numbers> {
  std::range_formatter<int> underlying_;
  constexpr formatter() {
    underlying_.set_brackets("<", ">");
    underlying_.set_separator("; ");
  }
  constexpr auto parse(std::format_parse_context& ctx) { return underlying_.parse(ctx); }
  constexpr auto format(const Numbers& n, std::format_context& ctx) const { return underlying_.format(n.values, ctx); }
};

CASE(range_formatter_, std::format("{}", Numbers{{1, 2, 3}}) == "<1; 2; 3>")


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
  assert(stack_());
  assert(queue_());
  assert(pqueue_());
  assert(vecbool_());
  assert(vecbool_ref());
  assert(vecbool_cref());
  assert(ill_utf8());
  assert(ill_utf8_2());
  assert(unicode_esc());
  assert(zero_width());
  assert(back_ins());
  assert(arr_iter());
  assert(cptr());
  assert(char_ptr());
  assert(char_arr());
  assert(sint_());
  assert(uint_());
  assert(int128());
  assert(uint128());
  assert(bases());
  assert(dyn_width());
  assert(dyn_both());
  assert(indexed());
  assert(bool_());
  assert(char_int());
  assert(str_prec());
  assert(range_iota());
  assert(range_trans());
  assert(range_chars());
  assert(range_str());
  assert(range_list());
  assert(range_nspec());
  assert(range_ms());
  assert(map_str());
  assert(tuple_nested());
  assert(tuple_m());
  assert(vformat_to());
#ifndef TEST_HAS_NO_WIDE_CHARACTERS
  assert(wide_debug_char());
  assert(wide_debug_string());
  assert(wide_nullptr());
  assert(widen_char());
  assert(wide_str());
  assert(wide_string());
  assert(wback_ins());
  assert(range_w());
  assert(pair_w());
  assert(fmt_to_n_w());
  assert(fsize_w());
  assert(wvformat());
#endif
  assert(range_formatter_());

  return 0;
}
