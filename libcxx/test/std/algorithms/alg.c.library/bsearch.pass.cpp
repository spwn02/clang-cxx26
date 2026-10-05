//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03

// <cstdlib>

// [alg.c.library]: bsearch has overloads for a const and a non-const base returning a pointer of the same constness.

#include <cassert>
#include <cstdlib>
#include <type_traits>

static int compare(const void* a, const void* b) { return *static_cast<const int*>(a) - *static_cast<const int*>(b); }

int main(int, char**) {
  int array[]        = {1, 3, 5, 7};
  const int carray[] = {1, 3, 5, 7};
  int key            = 5;
  static_assert(std::is_same<decltype(std::bsearch(&key, array, 4, sizeof(int), compare)), void*>::value, "");
  static_assert(std::is_same<decltype(std::bsearch(&key, carray, 4, sizeof(int), compare)), const void*>::value, "");
  assert(std::bsearch(&key, array, 4, sizeof(int), compare) == array + 2);
  assert(std::bsearch(&key, carray, 4, sizeof(int), compare) == carray + 2);
  int missing = 4;
  assert(std::bsearch(&missing, carray, 4, sizeof(int), compare) == nullptr);
  return 0;
}
