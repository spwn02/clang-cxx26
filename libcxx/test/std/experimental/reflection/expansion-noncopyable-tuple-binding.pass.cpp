//===----------------------------------------------------------------------===//
//
// Copyright 2026 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

// <experimental/reflection>
//
// [reflection]
//
// Regression test for upstream bloomberg/clang-p2996 issue #181: 'template
// for' over a tuple-like range with a non-copyable element (e.g.
// std::tuple<std::unique_ptr<int>, ...>) failed to bind for every expansion
// variable form (auto, auto&, auto&&, const auto&), not just the by-value
// 'auto' case that's expected to fail (a whole-tuple copy is inherent to
// by-value structured-binding-style decomposition -- the same as an
// ordinary 'auto [a, b, c, d] = tup;' on the same type, which fails
// identically; that's correct standard behavior, not this bug).
//
// Three independent, compounding defects, all in
// clang/lib/Sema/SemaExpand.cpp:
//
// 1. tryMakeCXXIterableExpansionSelectExpr's hidden '__range' was
//    unconditionally copy-constructed ('Range->getType().withConst()')
//    before this function had even determined whether the range type is
//    actually iterable (decided later, via begin()/end() lookup) -- so a
//    non-copyable type doomed *every* binding form with a hard, non-SFINAE
//    diagnostic before ever reaching the (correct, reference-based)
//    destructurable path below. Fixed by only copy-constructing when the
//    range type is genuinely copy-constructible (checked speculatively, not
//    assumed by a blanket switch to a forwarding reference -- see the
//    following point).
// 2. Even switching to a forwarding reference only when the range type
//    isn't copy-constructible, makeCXXDestructurableExpansionSelectExpr's
//    hidden binding type hardcoded 'SpelledAsLValue=true' when building a
//    reference type for the expansion variable, indistinguishable from
//    'auto&' -- so 'auto&&' over a genuine prvalue range failed to bind
//    (can't bind a plain lvalue reference to a temporary).
// 3. Neither path ever completed the range's type before doing a
//    begin()/end() member lookup on it (unlike ordinary range-based for,
//    which explicitly does), which asserts in an assertions-enabled build
//    when the range type is reached only via a reference parameter, never
//    otherwise odr-used before the loop.
//
// See docs/REFLECTION_CLOSEUP.md's item 5b write-up for the full root-cause
// account, including why the initial forwarding-reference fix for (1) had
// to be narrowed to only fire when the range type genuinely isn't
// copy-constructible: a naive unconditional switch to a reference
// regressed working, perfectly-copyable-range constexpr cases elsewhere in
// this same test suite (binding a reference to a temporary inside a
// manifestly-constant-evaluated context needs lifetime-extension bookkeeping
// that a plain by-value copy never needed).

#include <memory>
#include <meta>
#include <tuple>

using NCTuple = std::tuple<std::unique_ptr<int>, char, double, float>;

// --- destructurable path: all reference forms must bind, not copy. ---

int by_ref(NCTuple& tup) {
  int c = 0;
  template for (auto& elem : tup) { (void)elem; c += 1; }
  return c;
}
int by_rref_ref(NCTuple& tup) {
  int c = 0;
  template for (auto&& elem : tup) { (void)elem; c += 1; }
  return c;
}
int by_const_ref(NCTuple& tup) {
  int c = 0;
  template for (const auto& elem : tup) { (void)elem; c += 1; }
  return c;
}
// A genuinely-rvalue range, exercising the forwarding-reference '__range'
// path specifically (issue #181's original upstream repro shape).
int by_rref_prvalue() {
  int c = 0;
  template for (auto&& elem : NCTuple{}) { (void)elem; c += 1; }
  return c;
}
int by_const_source(const NCTuple& tup) {
  int c = 0;
  template for (const auto& elem : tup) { (void)elem; c += 1; }
  return c;
}
// auto& against a const source must deduce a const reference -- ordinary
// auto-deduction, not something this item is responsible for, but worth
// covering in the same translation unit as everything else per this file's
// "test all forms together" discipline (a discipline this exact item's
// investigation needed twice: the bugs above only ever surfaced once
// multiple binding forms were exercised in one TU, not in isolation).
int by_auto_ref_const_source(const NCTuple& tup) {
  int c = 0;
  template for (auto& elem : tup) { (void)elem; c += 1; }
  return c;
}

// const-correctness is actually enforced, not just accepted: elem must not
// be assignable when bound via 'const auto&'.
void const_is_enforced(std::tuple<int, int, int>& tup) {
  template for (const auto& elem : tup) {
    static_assert(!__is_assignable(decltype(elem)&, int));
  }
}

// --- iterable path: fixing (1)/(2) above must not regress this path,
// including inside a manifestly-constant-evaluated ('constexpr' expansion
// variable) context where the range is a copyable, genuinely-prvalue type
// produced by a real standard-library facility (this exact shape --
// std::define_static_array(std::meta::members_of(...)) bound via
// 'template for (constexpr auto ... : ...)' -- regressed during this fix's
// own development; see expansion-lambda-capture-crash.pass.cpp and
// deduction-guide-reflection-mangling.pass.cpp elsewhere in this directory,
// which independently exercise and guard the same underlying mechanism). ---

namespace n { int a; int b; }

consteval int count_members() {
  int c = 0;
  template for (constexpr auto m :
                std::define_static_array(std::meta::members_of(
                    ^^n, std::meta::access_context::unchecked()))) {
    (void)m;
    c += 1;
  }
  return c;
}
static_assert(count_members() >= 2);

int main(int, char**) {
  NCTuple t;
  by_ref(t);
  by_rref_ref(t);
  by_const_ref(t);
  by_rref_prvalue();
  by_const_source(t);
  by_auto_ref_const_source(t);
  return 0;
}
