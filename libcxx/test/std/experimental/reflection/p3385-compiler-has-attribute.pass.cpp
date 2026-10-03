// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest -fattribute-reflection
#include <meta>
#include <string_view>
struct S {};
// P3385R8 [meta.reflection.queries]: Both overloads throw unless is_attribute(a).
consteval bool rejects(bool flags) {
  try {
    if (flags) (void)std::meta::has_attribute(^^S, ^^int, std::meta::attribute_comparison::ignore_argument);
    else (void)std::meta::has_attribute(^^S, ^^int);
  } catch (const std::meta::exception& e) {
    return std::meta::identifier_of(e.from()) == "has_attribute";
  }
  return false;
}
static_assert(rejects(false));
static_assert(rejects(true));
static_assert(!std::meta::has_attribute(^^S, ^^[[nodiscard]]));
int main(int, char**) {}
