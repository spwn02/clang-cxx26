//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: clang-modules-build
// UNSUPPORTED: gcc

// XFAIL: has-no-cxx-module-support

// RUN: mkdir %t
// RUN: %{cxx} %{compile_flags} -std=c++26 -freflection-latest \
// RUN:     -Wno-reserved-module-identifier -Wno-reserved-user-defined-literal \
// RUN:     --precompile -o %t/std.pcm -c %{module-dir}/std.cppm
// RUN: %{cxx} %{compile_flags} %{link_flags} -std=c++26 -freflection-latest \
// RUN:     -fmodule-file=std=%t/std.pcm %t/std.pcm \
// RUN:     %s -o %t/std-algorithm-results.sh.cpp.tsk
// RUN: %{exec} %t/std-algorithm-results.sh.cpp.tsk

// [algorithm.syn]: ranges::shift_left, ranges::shift_right and the truncated result aliases are exported by the std
// module.
#include <cassert>

import std;

int main(int, char**) {
  std::vector<int> v{1, 2, 3, 4, 5};
  auto left = std::ranges::shift_left(v, 2);
  assert(left.begin() == v.begin() && left.end() == v.begin() + 3 && v[0] == 3 && v[2] == 5);
  std::vector<int> w{1, 2, 3, 4, 5};
  auto right = std::ranges::shift_right(w, 2);
  assert(right.begin() == w.begin() + 2 && right.end() == w.end() && w[2] == 1 && w[4] == 3);

  static_assert(std::same_as<std::ranges::reverse_copy_truncated_result<int*, long*>,
                             std::ranges::in_in_out_result<int*, int*, long*>>);
  static_assert(std::same_as<std::ranges::rotate_copy_truncated_result<int*, long*>,
                             std::ranges::in_in_out_result<int*, int*, long*>>);
  static_assert(std::same_as<std::ranges::set_difference_truncated_result<int*, long*, char*>,
                             std::ranges::in_in_out_result<int*, long*, char*>>);
  static_assert(std::same_as<decltype(std::bsearch(nullptr, static_cast<const void*>(nullptr), 0, 0, nullptr)),
                             const void*>);
  return 0;
}
