//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// <cstddef>

// NULL

// LWG4182: NULL is an implementation-defined null pointer constant that is a literal.

#include <cstddef>
#include <type_traits>

#include "test_macros.h"

#ifndef NULL
#  error NULL not defined
#endif

#if TEST_STD_VER >= 11 && defined(__clang__) && !defined(_MSC_VER) && !defined(__MINGW32__)
// clang defines NULL as the literal nullptr: a literal, usable as the sentinel of variadic functions.
static_assert(std::is_same<decltype(NULL), std::nullptr_t>::value, "");
#endif

int main(int, char**) {
  void* p = NULL;
  return p != NULL;
}
