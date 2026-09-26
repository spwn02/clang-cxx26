//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20
// UNSUPPORTED: no-threads

// <functional>, <future>

// LWG 3617: the deduction guides of std::function and std::packaged_task for a callable whose operator() has an
// explicit object parameter deduce the signature without the object parameter.

#include <cassert>
#include <functional>
#include <future>
#include <type_traits>

struct ByValue {
  int operator()(this ByValue, int x) { return x + 1; }
};
struct ByConstRef {
  int operator()(this const ByConstRef&, double, char) { return 2; }
};
struct NoArgs {
  int operator()(this const NoArgs&) noexcept { return 3; }
};
struct StaticOp {
  static int operator()(int a, int b) { return a + b; }
};
struct Regular {
  int operator()(long) const { return 5; }
};

static_assert(std::is_same_v<decltype(std::function{ByValue{}}), std::function<int(int)>>);
static_assert(std::is_same_v<decltype(std::function{ByConstRef{}}), std::function<int(double, char)>>);
static_assert(std::is_same_v<decltype(std::function{NoArgs{}}), std::function<int()>>);
static_assert(std::is_same_v<decltype(std::function{StaticOp{}}), std::function<int(int, int)>>);
static_assert(std::is_same_v<decltype(std::function{Regular{}}), std::function<int(long)>>);

static_assert(std::is_same_v<decltype(std::packaged_task{ByValue{}}), std::packaged_task<int(int)>>);
static_assert(std::is_same_v<decltype(std::packaged_task{ByConstRef{}}), std::packaged_task<int(double, char)>>);
static_assert(std::is_same_v<decltype(std::packaged_task{NoArgs{}}), std::packaged_task<int()>>);
static_assert(std::is_same_v<decltype(std::packaged_task{StaticOp{}}), std::packaged_task<int(int, int)>>);
static_assert(std::is_same_v<decltype(std::packaged_task{Regular{}}), std::packaged_task<int(long)>>);

int main(int, char**) {
  std::function f{ByValue{}};
  assert(f(41) == 42);
  std::function g{NoArgs{}};
  assert(g() == 3);
  std::packaged_task t{ByValue{}};
  auto future = t.get_future();
  t(9);
  assert(future.get() == 10);
  return 0;
}
