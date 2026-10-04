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
// RUN:     %s -o %t/std-const-iterators.sh.cpp.tsk
// RUN: %{exec} %t/std-const-iterators.sh.cpp.tsk

// [const.iterators], [range.refinements], [range.as.const]: the constant iterator and range
// facilities are exported by the std module.
import std;

int main(int, char**) {
  std::vector<int> v{1, 2, 3};
  static_assert(std::ranges::constant_range<const std::vector<int>>);
  static_assert(!std::ranges::constant_range<std::vector<int>>);
  static_assert(std::same_as<std::const_iterator<std::vector<int>::iterator>,
                             std::basic_const_iterator<std::vector<int>::iterator>>);
  static_assert(std::same_as<std::iter_const_reference_t<std::vector<int>::iterator>, const int&>);
  auto ci = std::make_const_iterator(v.begin());
  auto cs = std::make_const_sentinel(v.end());
  static_assert(std::same_as<std::iter_reference_t<decltype(ci)>, const int&>);
  int sum = 0;
  for (; ci != cs; ++ci)
    sum += *ci;
  auto cv = v | std::views::as_const;
  static_assert(std::ranges::constant_range<decltype(cv)>);
  static_assert(std::same_as<std::ranges::range_reference_t<decltype(cv)>, const int&>);
  return (sum == 6 && std::ranges::distance(cv) == 3) ? 0 : 1;
}
