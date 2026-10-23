//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11

// <algorithm>

// template<class ForwardIterator1, class ForwardIterator2>
//   bool is_permutation(ForwardIterator1 first1, ForwardIterator1 last1, ForwardIterator2 first2);
//
// Mandates: ForwardIterator1 and ForwardIterator2 have the same value type.

#include <algorithm>

#include "test_macros.h"

// Each call is diagnosed (the overload without a predicate delegates to the one with a predicate).
// expected-error@*:* 3+ {{ForwardIterator1 and ForwardIterator2 must have the same value type}}
void test() {
  int a[3] = {1, 2, 3};
  long b[3] = {3, 2, 1};
  (void)std::is_permutation(a, a + 3, b);
  (void)std::is_permutation(a, a + 3, b, [](int x, long y) { return x == y; });
#if TEST_STD_VER >= 14
  (void)std::is_permutation(a, a + 3, b, b + 3);
#endif
}
