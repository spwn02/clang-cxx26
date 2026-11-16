// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23, c++26
// ADDITIONAL_COMPILE_FLAGS: -fno-exceptions

#include <__meta/token_locations.h>
#include <meta>

constexpr auto __fix = std::meta::__x::__replace(
    std::meta::token{}, std::meta::token{}, "replacement");
static_assert(__fix.__operation ==
              std::meta::__x::__diagnostic_fixit::__kind::replace);
static_assert(__fix.__replacement == "replacement");

int main(int, char**) { return 0; }
