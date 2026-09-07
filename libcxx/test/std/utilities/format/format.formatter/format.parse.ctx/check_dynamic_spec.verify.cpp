//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// P2757R3: Type-checking format args
//
// template<class... Ts>
//   constexpr void check_dynamic_spec(size_t id) noexcept;
// constexpr void check_dynamic_spec_integral(size_t id) noexcept;
// constexpr void check_dynamic_spec_string(size_t id) noexcept;

#include <format>
#include <string>

// Mandates: the types in Ts... are unique.
void mandates_unique() {
  std::format_parse_context ctx("", 1);
  // expected-error@*:* {{the types in Ts... must be unique}}
  ctx.check_dynamic_spec<int, int>(0);
}

// Mandates: each type in Ts... is one of the twelve types the paper allows.
// std::string (as opposed to basic_string_view) isn't one of them. The
// Mandates violation makes the per-type mapping helper (itself consteval, to
// match __determine_arg_t's existing style) permanently unable to produce a
// constant, which escalates check_dynamic_spec<std::string> into an
// immediate function for this instantiation -- hence the second error below,
// in addition to the static_assert itself.
void mandates_allowed_type() {
  std::format_parse_context ctx("", 1);
  // expected-error@*:* {{check_dynamic_spec<Ts...>: each type in Ts... must be one of}}
  // expected-error@+1 {{call to immediate function}}
  ctx.check_dynamic_spec<std::string>(0);
}

// [format.parse.ctx]: a directly user-constructed context has no type
// information available (only the library's own compile-time format string
// validation threads that through), so check_dynamic_spec is never a core
// constant expression there -- same boundary as an out-of-range id.
constexpr bool direct_construction_always_fails() {
  std::format_parse_context ctx("", 5);
  ctx.check_dynamic_spec_integral(0);
  return true;
}

void f() {
  // expected-error@+1 {{static assertion expression is not an integral constant expression}}
  static_assert(direct_construction_always_fails());
}

// A dynamic width argument of the wrong type is rejected at the point the
// literal format string is converted to a format_string, i.e. compile time.
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

void wrong_dynamic_arg_type() {
  // expected-error@+1 {{call to consteval function}}
  (void)std::format("{:{}}", integral_width{5}, 3.5);
}
