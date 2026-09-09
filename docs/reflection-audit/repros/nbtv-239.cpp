#include <meta>
using namespace std::meta;
template <auto> struct sfinae_probe_value {};
template <std::meta::info Refl> consteval auto extract_closure_invoke_operator_refl() {
  constexpr auto closure_type_refl = Refl;
  using closure_t = typename[:closure_type_refl:];
  return ^^closure_t::operator();
}
template <std::meta::info Func> concept C = requires { typename sfinae_probe_value<extract_closure_invoke_operator_refl<parent_of(Func)>()>; };
consteval { constexpr auto r = ^^decltype([](this auto &&) {} )::operator(); static_assert(C<r>); }
