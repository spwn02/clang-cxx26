// RUN: %clang_cc1 -std=c++26 -freflection -fsyntax-only -Wunused-local-typedef -Wunused-variable -Wunused-parameter -Wunused-function -Wno-unneeded-internal-declaration -verify %s

// A local alias, variable, parameter or function named only as a reflection
// operand is referenced, so the -Wunused-* warnings must not fire for it.
using info = decltype(^^int);

template <class T> consteval bool in_template() {
  using Alias = T;
  constexpr info r = ^^Alias;
  return r == ^^T;
}

template <class T> consteval bool in_template_splice() {
  using Alias = T;
  using Same = [:^^Alias:];
  return sizeof(Same) != 0;
}

template <class T> consteval bool in_template_decltype() {
  using Alias = T;
  return decltype(^^Alias)(^^T) == ^^T;
}

consteval bool generic_lambda() {
  auto l = []<class T>(T) consteval {
    using Alias = T;
    return ^^Alias == ^^T;
  };
  return l(1);
}

consteval bool non_template() {
  using Alias = int;
  constexpr info r = ^^Alias;
  return r == ^^int;
}

template <class T> void really_unused() {
  using Unused = T; // expected-warning {{unused type alias 'Unused'}}
}

[[maybe_unused]] constexpr bool use_all = in_template<int>() && in_template_splice<int>() &&
                         in_template_decltype<int>() && generic_lambda() &&
                         non_template();
void instantiate() { really_unused<int>(); }

namespace {
consteval bool helper() { return true; }
consteval bool helper2() { return true; }
consteval bool never_named() { return true; } // expected-warning {{unused function 'never_named'}}
} // namespace

template <class T> consteval bool names_entities(int p) {
  int x = 0;
  constexpr info rx = ^^x;
  constexpr info rp = ^^p;
  constexpr info rh = ^^helper;
  return rx != rp && rh != rx;
}

consteval bool names_entities_plain(int p) {
  int x = 0;
  constexpr info rx = ^^x;
  constexpr info rp = ^^p;
  constexpr info rh = ^^helper2;
  return rx != rp && rh != rx;
}

consteval bool unused_controls(int p) { // expected-warning {{unused parameter 'p'}}
  int y = 0; // expected-warning {{unused variable 'y'}}
  return true;
}

[[maybe_unused]] constexpr bool use_entities = names_entities<int>(1) && names_entities_plain(1) &&
                              unused_controls(1);
