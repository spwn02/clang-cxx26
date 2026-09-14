// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <format>

#include <format>
#include <tuple>
#include <version>

consteval bool test_constexpr_format() {
  return std::format("bool={} char={} int={} string={} tuple={}", true, 'x', 42, "ok", std::tuple{1, 2}) ==
         "bool=true char=x int=42 string=ok tuple=(1, 2)";
}

static_assert(test_constexpr_format());
static_assert(__cpp_lib_constexpr_format == 202511L);
