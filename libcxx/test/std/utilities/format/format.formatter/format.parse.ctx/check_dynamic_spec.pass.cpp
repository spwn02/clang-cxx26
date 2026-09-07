//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <format>

// P2757R3: Type-checking format args
//
// template<class... Ts>
//   constexpr void check_dynamic_spec(size_t id) noexcept;
// constexpr void check_dynamic_spec_integral(size_t id) noexcept;
// constexpr void check_dynamic_spec_string(size_t id) noexcept;
//
// This lets a user-defined formatter's parse() validate the type of a
// dynamic width/precision argument at compile time, when the format string
// is known at compile time. The type information is only threaded through
// the parse context the library's own compile-time format string
// validation builds -- see check_dynamic_spec.verify.cpp for the boundary
// this implies for a directly user-constructed context.

#include <cassert>
#include <format>
#include <string_view>

#include "test_macros.h"

struct integral_width {
  int value;
};

template <>
struct std::formatter<integral_width> {
  constexpr auto parse(std::format_parse_context& ctx) {
    auto it = ctx.begin();
    if (it != ctx.end() && *it == '{') {
      ++it;
      ctx.check_dynamic_spec_integral(ctx.next_arg_id());
      if (it != ctx.end() && *it == '}')
        ++it;
    }
    while (it != ctx.end() && *it != '}')
      ++it;
    return it;
  }
  auto format(const integral_width& w, std::format_context& ctx) const {
    return std::format_to(ctx.out(), "{}", w.value);
  }
};

struct string_suffix {
  int value;
};

template <>
struct std::formatter<string_suffix> {
  constexpr auto parse(std::format_parse_context& ctx) {
    auto it = ctx.begin();
    if (it != ctx.end() && *it == '{') {
      ++it;
      ctx.check_dynamic_spec_string(ctx.next_arg_id());
      if (it != ctx.end() && *it == '}')
        ++it;
    }
    while (it != ctx.end() && *it != '}')
      ++it;
    return it;
  }
  auto format(const string_suffix& s, std::format_context& ctx) const {
    return std::format_to(ctx.out(), "{}", s.value);
  }
};

struct multi_type_spec {
  int value;
};

template <>
struct std::formatter<multi_type_spec> {
  constexpr auto parse(std::format_parse_context& ctx) {
    auto it = ctx.begin();
    if (it != ctx.end() && *it == '{') {
      ++it;
      // Accepts either an int or a double as the dynamic argument.
      ctx.check_dynamic_spec<int, double>(ctx.next_arg_id());
      if (it != ctx.end() && *it == '}')
        ++it;
    }
    while (it != ctx.end() && *it != '}')
      ++it;
    return it;
  }
  auto format(const multi_type_spec& s, std::format_context& ctx) const {
    return std::format_to(ctx.out(), "{}", s.value);
  }
};

int main(int, char**) {
  // check_dynamic_spec_integral: dynamic arg is int, must be accepted. The
  // literal format string is also validated at compile time (via
  // basic_format_string's consteval constructor) as part of this same call
  // -- if the dynamic arg's type were wrong, this wouldn't compile at all,
  // see check_dynamic_spec.verify.cpp.
  assert(std::format("{:{}}", integral_width{5}, 3) == "5");

  // check_dynamic_spec_string: dynamic arg is a string_view, must be accepted.
  assert(std::format("{:{}}", string_suffix{7}, std::string_view("x")) == "7");

  // check_dynamic_spec<int, double>: dynamic arg is a double, one of the two
  // accepted types.
  assert(std::format("{:{}}", multi_type_spec{9}, 1.5) == "9");

  return 0;
}
