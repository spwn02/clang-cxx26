//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <memory>

// shared_ptr

// size_t owner_hash() const noexcept; // since C++26

#include <memory>
#include <cassert>
#include "test_macros.h"

int main(int, char**) {
  const std::shared_ptr<int> p1(new int);
  const std::shared_ptr<int> p2 = p1;
  const std::weak_ptr<int> w1(p1);

  assert(p1.owner_hash() == p2.owner_hash());
  assert(p1.owner_hash() == w1.owner_hash());

  ASSERT_NOEXCEPT(p1.owner_hash());
  ASSERT_SAME_TYPE(decltype(p1.owner_hash()), std::size_t);

  return 0;
}
