//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: libcpp-has-no-incomplete-pstl, clang-modules-build, gcc
// XFAIL: has-no-cxx-module-support
// MODULE_DEPENDENCIES: std

import std;

int main(int, char**) {
  // [alg.swap]: "{first1 + M, first2 + M} for the overloads in namespace ranges."
  int values[] = {1, 2, 3, 4};
  int other[] = {9, 8};
  auto swapped = std::ranges::swap_ranges(std::execution::seq, values, other);
  if (swapped.in1 != values + 2 || swapped.in2 != other + 2 || values[2] != 3)
    return 1;
  // [alg.remove]: "{j, last} for the overloads in namespace ranges."
  auto removed = std::ranges::remove_if(std::execution::seq, values, [](int value) { return value == 9; });
  if (removed.begin() != values + 3 || removed.end() != values + 4)
    return 2;
  // [alg.remove]: "Copies the first N elements ... for which E(i) is false"
  // "Otherwise, {j, result_last}, for the algorithms in namespace ranges"
  int output[] = {-1};
  int source[] = {1, 2, 1, 3};
  auto copied = std::ranges::remove_copy(std::execution::seq, source, output, {1});
  if (copied.in != source + 3 || copied.out != output + 1 || output[0] != 2)
    return 3;
  auto copied_if = std::ranges::remove_copy_if(
      std::execution::seq, source, output, [](int value) { return value == 1; });
  if (copied_if.in != source + 3 || copied_if.out != output + 1 || output[0] != 2)
    return 4;
  return 0;
}
