//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <atomic>

// [atomics.fences], [atomics.order]: atomic_thread_fence, atomic_signal_fence and kill_dependency are constexpr (P3309R3).

#include <atomic>

constexpr bool test() {
  std::atomic_thread_fence(std::memory_order_seq_cst);
  std::atomic_signal_fence(std::memory_order_acquire);
  return std::kill_dependency(3) == 3;
}
static_assert(test());
