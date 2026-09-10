//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <stacktrace>

// template<class Allocator> struct hash<basic_stacktrace<Allocator>>;

#include <cassert>
#include <functional>
#include <stacktrace>
#include <unordered_set>

int main(int, char**) {
  std::stacktrace empty1;
  std::stacktrace empty2;
  assert(std::hash<std::stacktrace>()(empty1) == std::hash<std::stacktrace>()(empty2));

  std::stacktrace st   = std::stacktrace::current();
  std::stacktrace copy = st;
  assert(st == copy);
  assert(std::hash<std::stacktrace>()(st) == std::hash<std::stacktrace>()(copy));

  std::unordered_set<std::stacktrace> traces;
  traces.insert(st);
  traces.insert(copy);   // equal to st -- must not grow the set.
  traces.insert(empty1);
  assert(traces.size() == 2);
  assert(traces.count(st) == 1);
  assert(traces.count(empty1) == 1);

  return 0;
}
