// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23, c++26
// ADDITIONAL_COMPILE_FLAGS: -fno-exceptions

#include <__meta/token_locations.h>
#include <meta>

constexpr auto __location =
    std::meta::__x::__token_begin(std::meta::token{});
static_assert(__location.line() == 0);
static_assert(__location.column() == 0);
static_assert(__location.file_name()[0] == '\0');

int main(int, char**) { return 0; }
