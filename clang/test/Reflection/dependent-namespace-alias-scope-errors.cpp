//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Diagnostics for dependent namespace aliases used as scopes (GitHub #197).
//
// RUN: %clang_cc1 %s -std=c++23 -freflection -verify

using info = decltype(^^int);

namespace N {
template <typename> struct TT { using type = int; };
struct P { using t = int; };
} // namespace N

// The unqualified 'template' form of a dependent member template is an error
// (not a crash).
template <info R> void f() {
  namespace A = [:R:];
  using T = A::TT<int>; // expected-error {{use 'template' keyword to treat 'TT' as a dependent template name}}
}

// Errors are diagnosed at instantiation time.
template <info R> void g() {
  namespace A = [:R:];
  typename A::P::missing m; // expected-error {{no type named 'missing' in 'N::P'}}
}
template <info R> int h() {
  namespace A = [:R:];
  return A::nonexistent; // expected-error {{no member named 'nonexistent' in namespace 'N'}}
}
void use() {
  g<^^N>(); // expected-note {{in instantiation of function template specialization 'g<^^(namespace)>' requested here}}
  h<^^N>(); // expected-note {{in instantiation of function template specialization 'h<^^(namespace)>' requested here}}
}
