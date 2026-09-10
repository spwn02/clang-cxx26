//===----------------------------------------------------------------------===//
// M5 diagnostic coverage: P3491R3, P3560R2, and P3795R2
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

#include <array>
#include <meta>
#include <string>

namespace batch16 {

// 3491-04: each array element must be reflectable as a constant expression.
int runtime_value();
constexpr auto bad_array = std::define_static_array(
    std::array{runtime_value()});
// expected-error@-2 {{must be initialized by a constant expression}}

// 3795-04: generated-member annotations retain source annotation constraints.
struct Annotated;
consteval {
  define_aggregate(^^Annotated, {
      data_member_spec(^^int, {.name = "bad",
                               .annotations = {std::meta::reflect_constant(
                                   std::string{"not structural"})}})});
}
// expected-error@25 {{no matching function for call to 'reflect_constant'}}
// expected-error@22 {{evaluating expression of a consteval block must be a constant expression}}

} // namespace batch16
