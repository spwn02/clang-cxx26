//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <stdckdint.h>

// P3370R1: checked integer arithmetic, adopted from C23.
//
//   template<class type1, class type2, class type3>
//     bool ckd_add(type1* result, type2 a, type3 b);
//   template<class type1, class type2, class type3>
//     bool ckd_sub(type1* result, type2 a, type3 b);
//   template<class type1, class type2, class type3>
//     bool ckd_mul(type1* result, type2 a, type3 b);
//
// Each computes *result = a op b in infinite signed precision, converts that
// to type1, and returns true (the stored result is wrapped) iff the
// conversion isn't value-preserving.

#include <stdckdint.h>

#include <cassert>
#include <cstdint>
#include <limits>

#ifndef __STDC_VERSION_STDCKDINT_H__
#  error __STDC_VERSION_STDCKDINT_H__ should be defined
#endif
static_assert(__STDC_VERSION_STDCKDINT_H__ == 202311L, "");

int main(int, char**) {
  // No overflow.
  {
    int r;
    assert(!ckd_add(&r, 2, 2));
    assert(r == 4);

    assert(!ckd_sub(&r, 5, 2));
    assert(r == 3);

    assert(!ckd_mul(&r, 6, 7));
    assert(r == 42);
  }

  // Overflow, same-width unsigned result type.
  {
    unsigned int r;
    assert(ckd_add(&r, std::numeric_limits<unsigned int>::max(), 1u));
    assert(r == 0u);

    assert(ckd_sub(&r, 0u, 1u));
    assert(r == std::numeric_limits<unsigned int>::max());

    assert(ckd_mul(&r, std::numeric_limits<unsigned int>::max(), 2u));
  }

  // Overflow, signed result type.
  {
    int r;
    assert(ckd_add(&r, std::numeric_limits<int>::max(), 1));
    assert(ckd_sub(&r, std::numeric_limits<int>::min(), 1));
    assert(ckd_mul(&r, std::numeric_limits<int>::max(), 2));
  }

  // A negative operand doesn't fit an unsigned result type.
  {
    unsigned char r;
    assert(ckd_sub(&r, 1, 2));
  }

  // A too-wide value doesn't fit a narrower result type.
  {
    unsigned char r;
    assert(!ckd_add(&r, 100, 100));
    assert(r == 200);
    assert(ckd_add(&r, 200, 100));
  }

  // type1/type2/type3 need not be the same type.
  {
    long long r;
    assert(!ckd_add(&r, (int)1, (unsigned char)2));
    assert(r == 3);
  }

  return 0;
}
