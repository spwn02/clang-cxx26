//===----------------------------------------------------------------------===//
//
// Copyright 2026 CXX26 Clang contributors.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

#include <meta>

struct Base {
  int value;
};

struct Derived : Base {
  int own;
};

struct VirtualDerived : virtual Base {};

constexpr auto ctx = std::meta::access_context::unchecked();
constexpr auto base = std::meta::bases_of(^^Derived, ctx)[0];
constexpr auto virtual_base = std::meta::bases_of(^^VirtualDerived, ctx)[0];
constexpr auto not_a_base = ^^int;

int read(Derived& d) {
  return d.[:base:].value;
}

void write(Derived& d) {
  d.[:base:].value = 42;
}

void accepts_virtual(VirtualDerived& d) {
  (void)d.[:virtual_base:];
}

void accepts_array_element(Derived (&d)[1]) {
  (void)d[0].[:base:];
}

void rejects_non_base(Derived& d) {
  (void)d.[:not_a_base:]; // expected-error {{reflection not usable in a splice expression}}
}

int main() {
  Derived d{{7}, 11};
  if (read(d) != 7)
    return 1;
  write(d);
  return d.Base::value != 42;
}

struct Indirect : Base {};
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winaccessible-base"
struct Diamond : Base, Indirect {};
#pragma clang diagnostic pop
struct Private : private Base {};
struct Protected : protected Base {};
constexpr auto diamond_base = std::meta::bases_of(^^Diamond, ctx)[0];
constexpr auto private_base = std::meta::bases_of(^^Private, ctx)[0];
constexpr auto protected_base = std::meta::bases_of(^^Protected, ctx)[0];
int diamond(Diamond& d) { return d.[:diamond_base:].value; }
int private_access(Private& d) { return d.[:private_base:].value; }
int protected_access(Protected& d) { return d.[:protected_base:].value; }
int arrow(Derived* d) { return d->[:base:].value; }
template<class T, auto R> int dependent(T& t) { return t.[:R:].value; }
int instantiate(Derived& d) { return dependent<Derived, base>(d); }
template<class T> struct TemplateDerived : Derived {
  int get() { return this->[:std::meta::bases_of(^^TemplateDerived, ctx)[0]:].value; }
};
int instantiate_this() { TemplateDerived<int> d{}; return d.get(); }
struct Other {};
int unrelated(Other& o) {
  return o.[:base:].value; // expected-error {{not derived from splice class}}
}
void standalone() {
  (void)[:base:]; // expected-error {{must be the second operand of a member access}}
  (void)&[:base:]; // expected-error {{must be the second operand of a member access}}
  (void)sizeof([:base:]); // expected-error {{must be the second operand of a member access}}
  (void)sizeof(decltype([:base:])); // expected-error {{must be the second operand of a member access}}
}
template<auto R> void dependent_standalone() {
  (void)&[:R:]; // expected-error {{must be the second operand of a member access}}
}
template void dependent_standalone<base>();
template<class T, auto R> int dependent_unrelated(T& t) {
  return t.[:R:].value; // expected-error {{not derived from splice class}}
}
template int dependent_unrelated<Other, base>(Other&);
static_assert(__is_same(decltype((Derived{}.[:base:])), Base&&));
static_assert(__is_same(decltype((std::declval<Derived&>().[:base:])), Base&));
template<class T> int known_base(T& t) { return t.[:base:].value; }
template int known_base<Derived>(Derived&);
template<class T> int dependent_arrow(T* t) { return t->[:base:].value; }
template int dependent_arrow<Derived>(Derived*);
template<class T> void known_standalone() {
  (void)[:base:]; // expected-error {{must be the second operand of a member access}}
}
