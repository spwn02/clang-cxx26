#include <meta>
using namespace std::meta;
consteval {
  constexpr auto closure = []() { };
  constexpr auto cr = ^^decltype(closure);
  using ct = typename[:cr:];
  static_assert(is_type_alias(^^ct));
  static_assert(has_identifier(^^ct));
}
