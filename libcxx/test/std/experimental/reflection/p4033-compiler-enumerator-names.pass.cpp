// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest -fattribute-reflection
#include <meta>
#include <string_view>
// P4033R1 [meta.reflection.enumerator.spec]: Throws unless name.value()
// is a valid identifier that is not a keyword.
consteval bool rejects(const char* name) {
  try { (void)std::meta::enumerator_spec({.name=name}); }
  catch (const std::meta::exception& e) {
    return std::meta::identifier_of(e.from()) == "enumerator_spec";
  }
  return false;
}
static_assert(rejects("class"));
static_assert(rejects("a b"));
static_assert(rejects(""));
static_assert(std::meta::is_enumerator_spec(std::meta::enumerator_spec({.name="final"})));
static_assert(std::meta::is_enumerator_spec(std::meta::enumerator_spec({.name="override"})));
static_assert(std::meta::is_enumerator_spec(std::meta::enumerator_spec({.name="é"})));
int main(int, char**) {}
