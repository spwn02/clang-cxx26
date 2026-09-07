//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: gcc-15, apple-clang-17

// <type_traits>

// LWG4138 <https://cplusplus.github.io/LWG/issue4138>
// std::is_within_lifetime shouldn't work when a function type is
// explicitly specified, even if it isn't evaluated

#include <type_traits>

template <class T>
consteval bool checked_is_within_lifetime(T* p) {
  // expected-error@+1 {{no matching function for call to 'is_within_lifetime'}}
  return p ? std::is_within_lifetime<T>(p) : false;
}
static_assert(!checked_is_within_lifetime<int>(nullptr));
// expected-error@+1 {{static assertion expression is not an integral constant expression}}
static_assert(!checked_is_within_lifetime<void()>(nullptr));
