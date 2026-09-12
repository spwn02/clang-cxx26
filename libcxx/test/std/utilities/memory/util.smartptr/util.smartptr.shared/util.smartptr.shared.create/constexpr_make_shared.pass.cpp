//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <memory>

#include <memory>

constexpr bool test() {
  auto p = std::make_shared<int>(42);
  if (!p || *p != 42 || p.use_count() != 1)
    return false;

  auto q = p;
  if (p.use_count() != 2 || q.get() != p.get())
    return false;

  q.reset();
  return p.use_count() == 1;
}

static_assert(test());

int main(int, char**) { return !test(); }
