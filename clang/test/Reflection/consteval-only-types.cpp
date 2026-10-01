//===----------------------------------------------------------------------===//
//
// Copyright 2025 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// RUN: %clang_cc1 %s -std=c++26 -freflection -verify

using info = decltype(^^int);
struct S { info m = {}; };

                                 // ===========
                                 // valid_cases
                                 // ===========

namespace valid_cases {
constexpr info r1 = ^^int;
static constexpr info r2 = ^^int;

constexpr S s1;
constexpr S s2{};
constexpr S s3 = {^^int};

constexpr info *p1 = nullptr;
constexpr const info *p2 = &r1;

void fn1() { static constexpr info r = ^^int; }
void fn2() { extern info r; }
consteval info cfn1() { return ^^int; }
consteval void cfn2() { (void) static_cast<const void *>(p2); }

}  // namespace valid_cases


                           // ======================
                           // non_consteval_contexts
                           // ======================

namespace non_consteval_contexts {
// Null reflections have no consteval-only constituent value ([expr.const]).
info r1;
info r2 {};
info r3 = ^^int;
// expected-error@-1 {{is not associated with a constexpr variable}}
info r4 = valid_cases::cfn1();
// expected-error@-1 {{is not associated with a constexpr variable}}
unsigned sz = sizeof(^^int);  // ok

// Aggregates containing only null reflections are ordinary objects.
S s1;
S s2{};
S s3 = {^^int};
// expected-error@-1 {{is not associated with a constexpr variable}}

// A zero-initialized pointer does not establish an immediate object.
const info *p1;
const info *p2 = &valid_cases::r1;
// expected-error@-1 {{is not associated with a constexpr variable}}

info fn1() { return ^^int; }
// expected-error@-1 {{consteval-only value is only allowed}}

info fn2() { return valid_cases::cfn1(); }
// expected-error@-1 {{consteval-only value is only allowed}}

void fn3() { (void) valid_cases::r1; }
// expected-error@-1 {{consteval-only value is only allowed}}

// s1 contains a null reflection, so this does not use an immediate object.
void fn4() { (void) valid_cases::s1.m; }
void fn4_immediate() { (void) valid_cases::s3.m; }
// expected-error@-1 {{consteval-only value is only allowed}}

void fn5() { (void) static_cast<const void *>(valid_cases::p2); }
// expected-error@-1 {{consteval-only value is only allowed}}

void fn6() { (void) [:^^valid_cases::r1:]; }
// expected-error@-1 {{consteval-only value is only allowed}}

void fn7() {
  // Null reflections are permitted at runtime, including in allocated objects.
  (void) info{};
  (void) ^^int; // expected-error {{consteval-only value is only allowed}}
  (void) new info{};
}

consteval bool is_null(info R) {
  return R == info{} ? 1 : 0;
}

template <info R>
void fn() {
  constexpr auto S = R;
  (void) is_null(S);
}

}  // namespace non_consteval_contexts


                            // ====================
                            // immediate_escalation
                            // ====================

namespace immediate_escalation {
void fn() {
  [] { info r1; }();
  [] { info r2 {}; }();
  [] { info r3 = ^^int; }();
  [] { info r4 = valid_cases::cfn1(); }();

  [] { S s1; }();
  [] { S s2{}; }();
  [] { S s3 = {^^int}; }();

  [] { const info *p1; }();
  [] { const info *p2 = &valid_cases::r1; }();

  (void) [] { return ^^int; }();
  (void) [] { return valid_cases::cfn1(); }();
  [] { (void) valid_cases::r1; }();
  [] { (void) valid_cases::s1.m; }();
  [] { (void) static_cast<const void *>(valid_cases::p2); }();
  [] { (void) [:^^valid_cases::r1:]; }();

  [] { (void) info{}; }();
  [] { (void) ^^int; }();
  [] { (void) new info{}; }();
}

}  // namespace immediate_escalation


                               // ===============
                               // alias_smuggling
                               // ===============

namespace alias_smuggling {
struct Base { };
struct Derived : Base {
  info k;
  consteval Derived() : Base(), k(^^int) {}
};
consteval const Base &fn1() {
  static constexpr Derived d;
  return d;
}
// [expr.const]: constexpr references may refer to immediate objects,
// including an empty base subobject of an immediate complete object.
constexpr auto &ref = fn1();
const Base &use1() { return ref; } // expected-error {{consteval-only value is only allowed}}

consteval void *fn2() {
  static constexpr auto v = ^^int;
  return (void *)&v;
}
// [expr.const]: constexpr objects may contain consteval-only pointers;
// erasing the pointee type does not permit their values to escape at runtime.
constexpr const void *ptr = fn2();
const void *use2() { return ptr; } // expected-error {{consteval-only value is only allowed}}

}  // namespace alias_smuggling


                            // =======================
                            // destructor_escalation
                            // =======================

// A destructor whose body handles a consteval-only value (directly, or via
// a base/member) must still be able to become an immediate function like
// any other member of a consteval-only class template specialization, so a
// container of a consteval-only type (e.g. std::vector<std::meta::info>)
// can be destroyed inside a manifestly constant-evaluated context. Plain,
// non-template destructors remain excluded per [expr.const]p17, unaffected
// by this.
namespace destructor_escalation {

template <typename T>
struct Holder {
  T m;
  constexpr ~Holder() { (void) m; }
};

consteval bool fn() {
  Holder<info> h{^^int};
  return true;
}
static_assert(fn());

template <typename T>
struct Wrapper {
  Holder<T> h;
  constexpr ~Wrapper() {}
};

consteval bool fn2() {
  Wrapper<info> w{{^^int}};
  return true;
}
static_assert(fn2());

}  // namespace destructor_escalation
