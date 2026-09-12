//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: no-exceptions
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <exception>

// template<class E> void exception_ptr_cast(const exception_ptr&&) = delete;
//
// A temporary exception_ptr must not bind to the lvalue-reference overload:
// the returned optional<const E&> would alias the temporary's exception
// object, which is about to be destroyed.

#include <exception>

struct E {};

void f(std::exception_ptr p) {
  std::exception_ptr_cast<E>(std::exception_ptr(p)); // expected-error {{call to deleted function 'exception_ptr_cast'}}
  std::exception_ptr_cast<E>(std::move(p));           // expected-error {{call to deleted function 'exception_ptr_cast'}}
}
