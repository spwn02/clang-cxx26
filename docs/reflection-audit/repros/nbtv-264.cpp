#include <meta>
struct Value { int field; };
constexpr Value value{42};
consteval int direct() {
  constexpr auto field = std::meta::nonstatic_data_members_of(^^Value, std::meta::access_context::unchecked())[0];
  return (&value)->[:field:];
}
static_assert(direct() == 42);
