// RUN: %clang_cc1 -std=c++26 -freflection -fsyntax-only -Wunused-local-typedef -verify %s

// A local alias used only as a reflection operand is a use of that alias.
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

constexpr bool use_all = in_template<int>() && in_template_splice<int>() &&
                         in_template_decltype<int>() && generic_lambda() &&
                         non_template();
void instantiate() { really_unused<int>(); }
