//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <execution>

// LWG4445: task_scheduler only declares its copy operations, so "moving" one copies it and the
// moved-from object still refers to its scheduler.

#include <cassert>
#include <execution>
#include <type_traits>
#include <utility>

namespace ex = std::execution;

int main(int, char**) {
  ex::task_scheduler a(ex::inline_scheduler{});
  ex::task_scheduler b = std::move(a);
  assert(a == b);
  (void)ex::schedule(a);

  ex::task_scheduler c(ex::inline_scheduler{});
  c = std::move(b);
  assert(b == c);
  (void)ex::schedule(b);

  static_assert(std::is_copy_constructible_v<ex::task_scheduler>);
  static_assert(std::is_copy_assignable_v<ex::task_scheduler>);
  return 0;
}
