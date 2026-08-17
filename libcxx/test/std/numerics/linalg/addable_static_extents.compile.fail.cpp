//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <linalg>

// possibly-addable is a Mandates and compares every pair of operands, not a
// chain through one of them. Here the left operand has a dynamic extent, so
// left/right and left/output are both compatible, but right (3) and output (5)
// are statically incompatible and must be diagnosed.

#include <linalg>

void f() {
  double l[5]{}, r[5]{}, o[5]{};
  std::mdspan<double, std::extents<size_t, std::dynamic_extent>> left(l, 3);
  std::mdspan<double, std::extents<size_t, 3>> right(r);
  std::mdspan<double, std::extents<size_t, 5>> out(o);

  std::linalg::add(left, right, out);
}
