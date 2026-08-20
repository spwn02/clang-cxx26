//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <memory>

// weak_ptr

// template <class U> bool owner_equal(shared_ptr<U> const& b) const noexcept; // since C++26
// template <class U> bool owner_equal(weak_ptr<U> const& b) const noexcept;   // since C++26

#include <memory>
#include <cassert>
#include "test_macros.h"

int main(int, char**) {
  const std::shared_ptr<int> p1(new int);
  const std::shared_ptr<int> p3(new int);
  const std::weak_ptr<int> w1(p1);
  const std::weak_ptr<int> w1b(p1);
  const std::weak_ptr<int> w3(p3);

  assert(w1.owner_equal(w1b));
  assert(!w1.owner_equal(w3));
  assert(w1.owner_equal(p1));
  assert(!w1.owner_equal(p3));

  ASSERT_NOEXCEPT(w1.owner_equal(w1b));
  ASSERT_NOEXCEPT(w1.owner_equal(p1));
  ASSERT_SAME_TYPE(decltype(w1.owner_equal(w1b)), bool);

  return 0;
}
