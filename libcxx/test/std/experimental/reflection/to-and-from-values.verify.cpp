//===----------------------------------------------------------------------===//
//
// Copyright 2024 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20
// ADDITIONAL_COMPILE_FLAGS: -freflection

// A failing metafunction throws std::meta::exception; the constant evaluator
// adds the reason as a "reflection failure" note (spwn02/clang-cxx26#126).

// <experimental/reflection>
//
// [reflection]

#include <meta>


                             // ==================
                             // disallowed_results
                             // ==================

namespace disallowed_results {
constexpr auto v1 = std::meta::reflect_constant((const char *)"fails");
  // expected-error@-1 {{must be initialized by a constant expression}} \
  // expected-note@* {{exception thrown here was not caught within the constant expression}}
  // expected-note@* {{provided value cannot be represented by a reflection}}

struct HoldsTemporary {
  const int &tmp;
};
constexpr HoldsTemporary htmp{42};
constexpr auto v2 = std::meta::reflect_constant(htmp);
  // expected-error@-1 {{must be initialized by a constant expression}} \
  // expected-note@* {{exception thrown here was not caught within the constant expression}}
  // expected-note@* {{provided value cannot be represented by a reflection}}

}  // namespace disallowed_results

                              // ================
                              // self_referential
                              // ================

namespace self_referential {
struct A {
  int *const p;
  consteval A(int *p) : p(p) {}
  consteval A(const A &oth) : p(0) {
    if (oth.p) {
      ++*oth.p;
    }
  }
};

consteval int f() {
  int x = 42;
  std::meta::reflect_constant<A>(&x);
  return x;
}

static_assert(f() == 43);
  // expected-error@-1 {{not an integral constant expression}}
}  // namespace self_referential

int main() { }
