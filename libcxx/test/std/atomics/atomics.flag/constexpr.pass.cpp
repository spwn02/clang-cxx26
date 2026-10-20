//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <atomic>

// [atomics.flag]: test, test_and_set, clear, wait, notify_one and notify_all of atomic_flag
// are constexpr. Constant evaluation is single-threaded: wait returns at once when the value
// differs and notifications have nothing to wake up.

#include <atomic>
#include <cassert>

constexpr bool test() {
  std::atomic_flag f;
  assert(!f.test());
  assert(!f.test_and_set());
  assert(f.test());
  assert(f.test_and_set(std::memory_order_acquire));
  f.wait(false);
  f.notify_one();
  f.notify_all();
  f.clear();
  assert(!f.test(std::memory_order_relaxed));
  f.wait(true);
  f.clear(std::memory_order_release);
  return true;
}

int main(int, char**) {
  test();
  static_assert(test());

  // The runtime behavior is unchanged.
  std::atomic_flag g;
  assert(!g.test_and_set());
  g.wait(false);
  g.notify_all();
  return 0;
}
