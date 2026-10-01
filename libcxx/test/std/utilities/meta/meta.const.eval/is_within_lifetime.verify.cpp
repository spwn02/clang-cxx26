//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <type_traits>

// template<class T>
//   consteval bool is_within_lifetime(const T* p) noexcept; // C++26
//
// Constraints: is_function_v<T> is false.

#include <type_traits>

void f();

void test() {
  // Explicit template argument forces T = void() rather than letting
  // deduction against `const T*` fail first, matching how
  // clang/test/SemaCXX/builtin-is-within-lifetime.cpp exercises the same
  // constraint. The rejection notes ("constraints not satisfied ...
  // because '!is_function_v<void ()>' evaluated to false") live inside
  // <type_traits> itself, not this file, so only the top-level diagnostic
  // is checked here to avoid coupling this test to the header's exact
  // line numbers.
  // expected-error@+1 {{no matching function for call to 'is_within_lifetime'}}
  std::is_within_lifetime<void()>(&f);
}

// [expr.const]: an immediate function pointer makes its containing object
// immediate. This distinguishes a consteval function from a constexpr function.
constexpr auto permitted = &std::is_within_lifetime<int>;
template <auto> struct Address {};
Address<&std::is_within_lifetime<int>> argument;
auto runtime = &std::is_within_lifetime<int>; // expected-error {{immediate object associated with variable 'runtime' is not associated with a constexpr variable}}

// Check the consteval-propagating property as well.
template <typename T>
constexpr void does_escalate(T p) {
  (void)std::is_within_lifetime(p);
}
constexpr auto propagated = &does_escalate<int*>;
Address<&does_escalate<int*>> propagated_argument;
auto runtime_propagated = &does_escalate<int*>; // expected-error {{immediate object associated with variable 'runtime_propagated' is not associated with a constexpr variable}}
