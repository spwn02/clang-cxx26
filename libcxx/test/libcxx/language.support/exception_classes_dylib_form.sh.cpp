//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// In C++26 the members of the exception classes that libc++abi defines (exception, bad_exception, bad_alloc,
// bad_array_new_length, bad_cast, bad_typeid) are inline constexpr, so that they can be thrown in constant expressions.
// The translation units of libc++ on Apple's platforms (libc++.dylib re-exports libc++abi.dylib, which defines these
// members) use the out-of-line declarations instead, selected by _LIBCPP_DISABLE_INLINE_EXCEPTION_MEMBERS.

// RUN: %{cxx} %s %{flags} %{compile_flags} -fsyntax-only
// RUN: %{cxx} %s %{flags} %{compile_flags} -fsyntax-only -Xclang -verify=disabled -Xclang -verify-ignore-unexpected=note -D_LIBCPP_DISABLE_INLINE_EXCEPTION_MEMBERS

#include <exception>
#include <new>
#include <typeinfo>

constexpr bool construct_and_destroy() {
  std::exception e;
  std::bad_exception be;
  std::bad_alloc ba;
  std::bad_array_new_length banl;
  std::bad_cast bc;
  std::bad_typeid bt;
  return true;
}

// With the inline constexpr members this is a constant expression; with the out-of-line declarations it is not.
static_assert(construct_and_destroy()); // disabled-error {{static assertion expression is not an integral constant expression}}
