//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <inplace_vector>

// Trivial element types can be constructed, mutated, and destroyed entirely
// at compile time, since storage for them is a plain array rather than
// lazily-activated raw bytes.

#include <inplace_vector>

#include "test_macros.h"

constexpr bool test() {
  std::inplace_vector<int, 4> v;
  v.push_back(1);
  v.push_back(2);
  v.emplace_back(3);
  if (v.size() != 3 || v[0] != 1 || v[1] != 2 || v[2] != 3)
    return false;

  v.insert(v.begin() + 1, 99);
  if (v.size() != 4 || v[1] != 99)
    return false;

  v.erase(v.begin());
  if (v.size() != 3 || v[0] != 99)
    return false;

  v.pop_back();
  if (v.size() != 2)
    return false;

  std::inplace_vector<int, 4> v2 = v;
  if (v2 != v)
    return false;

  v.clear();
  if (!v.empty())
    return false;

  return true;
}
static_assert(test());

static_assert(std::inplace_vector<int, 0>{}.empty());
static_assert(std::inplace_vector<int, 0>::capacity() == 0);

int main(int, char**) { return 0; }
