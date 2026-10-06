//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <type_traits>

// template<class U = void, class T>
//   consteval bool is_within_lifetime(const T* p) noexcept; // C++26
//
// Mandates: static_cast<const volatile U*>(p) is well-formed.

#include <type_traits>

void f();
struct Unrelated {};

void test() {
  // const T* cannot be deduced from a function pointer
  (void)std::is_within_lifetime(&f); // expected-error {{no matching function for call to 'is_within_lifetime'}}

  int i = 0;
  // U has to be related to the pointee: the Mandates element
  // expected-error@*:* {{is_within_lifetime requires static_cast<const volatile U*>(p) to be well-formed}}
  (void)std::is_within_lifetime<Unrelated>(&i); // expected-error {{call to consteval function 'std::is_within_lifetime<Unrelated, int>' is not a constant expression}}
}

// [expr.const]: an immediate function pointer makes its containing object
// immediate. This distinguishes a consteval function from a constexpr function.
constexpr auto permitted = &std::is_within_lifetime<void, int>;
template <auto> struct Address {};
Address<&std::is_within_lifetime<void, int>> argument;
auto runtime = &std::is_within_lifetime<void, int>; // expected-error {{immediate object associated with variable 'runtime' is not associated with a constexpr variable}}

// Check the consteval-propagating property as well.
template <typename T>
constexpr void does_escalate(T p) {
  (void)std::is_within_lifetime(p);
}
constexpr auto propagated = &does_escalate<int*>;
Address<&does_escalate<int*>> propagated_argument;
auto runtime_propagated = &does_escalate<int*>; // expected-error {{immediate object associated with variable 'runtime_propagated' is not associated with a constexpr variable}}
