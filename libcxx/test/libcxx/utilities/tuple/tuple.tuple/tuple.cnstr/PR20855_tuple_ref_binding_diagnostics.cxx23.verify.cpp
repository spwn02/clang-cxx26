//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03

// REQUIRES: std-at-least-c++23

// <tuple>

// Since P2255R2 the constructors that would dangle are deleted, so the diagnostics are emitted at the call site.

// See https://llvm.org/PR20855

#include <tuple>
#include <string>

#include "test_macros.h"

template <class Tp>
struct ConvertsTo {
  using RawTp = typename std::remove_cv< typename std::remove_reference<Tp>::type>::type;

  operator Tp() const {
    return static_cast<Tp>(value);
  }

  mutable RawTp value;
};

struct Base {};
struct Derived : Base {};

template <class T> struct CannotDeduce {
 using type = T;
};

template <class ...Args>
void F(typename CannotDeduce<std::tuple<Args...>>::type const&) {}

void f() {
  {
    F<int, const std::string&>(std::make_tuple(1, "abc")); // expected-error {{invokes a deleted function}}
  }
  {
    std::tuple<int, const std::string&> t(1, "a"); // expected-error {{call to deleted constructor}}
  }
  {
    F<int, const std::string&>(std::tuple<int, const std::string&>(1, "abc")); // expected-error {{call to deleted constructor}}
  }
  {
    ConvertsTo<int&> ct;
    std::tuple<const long&, int> t(ct, 42); // expected-error {{call to deleted constructor}}
  }
  {
    ConvertsTo<int> ct;
    std::tuple<int const&, void*> t(ct, nullptr); // expected-error {{call to deleted constructor}}
  }
  {
    ConvertsTo<Derived> ct;
    std::tuple<Base const&, int> t(ct, 42); // expected-error {{call to deleted constructor}}
  }
  {
    std::allocator<int> alloc;
    std::tuple<std::string &&> t2("hello"); // expected-error {{call to deleted constructor}}
    std::tuple<std::string &&> t3(std::allocator_arg, alloc, "hello"); // expected-error {{call to deleted constructor}}
  }
}
