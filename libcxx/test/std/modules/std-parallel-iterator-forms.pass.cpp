//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03, c++11, c++14
// UNSUPPORTED: libcpp-has-no-incomplete-pstl, clang-modules-build, gcc
// XFAIL: has-no-cxx-module-support
// MODULE_DEPENDENCIES: std

// The execution-policy overloads of remove_copy, remove_copy_if, unique_copy and is_sorted_until are exported by the
// std module.

import std;

int main(int, char**) {
  int in[]  = {1, 1, 2, 3, 3};
  int out[] = {0, 0, 0, 0, 0};

  auto r1 = std::remove_copy(std::execution::par, in, in + 5, out, 1);
  if (r1 != out + 3 || out[0] != 2 || out[1] != 3 || out[2] != 3)
    return 1;
  auto r2 = std::remove_copy_if(std::execution::par, in, in + 5, out, [](int v) { return v == 3; });
  if (r2 != out + 3 || out[0] != 1 || out[1] != 1 || out[2] != 2)
    return 2;
  auto r3 = std::unique_copy(std::execution::par, in, in + 5, out);
  if (r3 != out + 3 || out[0] != 1 || out[1] != 2 || out[2] != 3)
    return 3;
  auto r4 = std::unique_copy(std::execution::par, in, in + 5, out, [](int a, int b) { return a == b; });
  if (r4 != out + 3)
    return 4;
  int unsorted[] = {1, 2, 5, 4};
  if (std::is_sorted_until(std::execution::par, unsorted, unsorted + 4) != unsorted + 3)
    return 5;
  if (std::is_sorted_until(std::execution::par, unsorted, unsorted + 4, std::greater<int>{}) != unsorted + 1)
    return 6;
  return 0;
}
