// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest -fattribute-reflection
#include <meta>
#include <string_view>
// P3385R8 [meta.reflection.names]: Returns the attribute-token.
static_assert(std::meta::has_identifier(^^[[nodiscard]]));
static_assert(std::meta::identifier_of(^^[[nodiscard]]) == "nodiscard");
static_assert(std::meta::u8identifier_of(^^[[nodiscard]]) == u8"nodiscard");
static_assert(std::meta::has_identifier(^^[[clang::always_inline]]));
static_assert(std::meta::identifier_of(^^[[clang::always_inline]]) == "clang::always_inline");
static_assert(std::meta::u8identifier_of(^^[[clang::always_inline]]) == u8"clang::always_inline");
int main(int, char**) {}
