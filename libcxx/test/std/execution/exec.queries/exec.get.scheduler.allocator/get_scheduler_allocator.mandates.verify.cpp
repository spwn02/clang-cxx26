//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <execution>

#include <execution>

struct InvalidAllocatorEnv {
  int query(std::get_scheduler_allocator_t) const noexcept { return 0; }
};

int main(int, char**) {
  (void)std::get_scheduler_allocator(
      InvalidAllocatorEnv{}); // expected-error@*:* {{static assertion failed: Mandates: the type of env.query(get_scheduler_allocator) satisfies simple-allocator.}}
}
