//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef TEST_SUPPORT_IS_PERMUTATION_OF_INTS_H
#define TEST_SUPPORT_IS_PERMUTATION_OF_INTS_H

#include <iterator>

#include "test_macros.h"

// std::is_permutation requires both ranges to have the same value type
// ([alg.is.permutation] Mandates). The sorting tests compare a range of some
// element type T against an array of int, so count occurrences instead.
template <class Iter>
TEST_CONSTEXPR_CXX14 bool is_permutation_of_ints(Iter first, Iter last, const int* orig) {
  const int n = static_cast<int>(std::distance(first, last));
  for (Iter i = first; i != last; ++i) {
    int in_work = 0;
    int in_orig = 0;
    for (Iter j = first; j != last; ++j)
      if (*j == *i)
        ++in_work;
    for (int k = 0; k < n; ++k)
      if (*i == orig[k])
        ++in_orig;
    if (in_work != in_orig)
      return false;
  }
  return true;
}

#endif // TEST_SUPPORT_IS_PERMUTATION_OF_INTS_H
