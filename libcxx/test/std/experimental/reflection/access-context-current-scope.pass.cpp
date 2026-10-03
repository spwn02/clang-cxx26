//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

#include <meta>

using namespace std::meta;

// [meta.reflection.access.context], including every assertion in the example.
struct A {
  int a = 0;
  consteval A(int p) : a(p) {}
};
struct B : A {
  using A::A;
  consteval B(int p, int q) : A(p * q) {}
  info s = access_context::current().scope();
};
struct C : B { using B::B; };

struct Agg {
  consteval bool eq(info rhs = access_context::current().scope()) {
    return s == rhs;
  }
  info s = access_context::current().scope();
};

namespace NS {
  static_assert(Agg{}.s == access_context::current().scope());
  static_assert(Agg{}.eq());
  static_assert(B(1).s == ^^B);
  static_assert(is_constructor(B{1, 2}.s) && parent_of(B{1, 2}.s) == ^^B);
  static_assert(is_constructor(C{1, 2}.s) && parent_of(C{1, 2}.s) == ^^B);

  auto fn() -> [:is_namespace(access_context::current().scope()) ? ^^int : ^^bool:];
  static_assert(type_of(^^fn) == ^^auto()->int);

  template<auto R>
    struct TCls {
      consteval bool fn()
        requires (is_type(access_context::current().scope())) {
          return true;                  // OK, scope is TCls<R>.
        }
    };
  static_assert(TCls<0>{}.fn());
}

namespace extra {
template <auto R> struct TCls {
  consteval bool exact() requires (access_context::current().scope() == ^^TCls) {
    return is_function(access_context::current().scope());
  }
  template <class T>
  consteval bool member_template()
    requires (access_context::current().scope() == ^^TCls) {
    return is_function(access_context::current().scope());
  }
  consteval bool lambda()
    requires ([] { return is_function(access_context::current().scope()); }()) {
    return true;
  }
};
static_assert(TCls<1>{}.exact());
static_assert(TCls<1>{}.member_template<int>());
static_assert(TCls<1>{}.lambda());

consteval bool constrained_lambda() {
  return []<class T>(T) requires (is_function(access_context::current().scope())) {
    return true;
  }(0);
}
static_assert(constrained_lambda());
}

int main(int, char**) {}
