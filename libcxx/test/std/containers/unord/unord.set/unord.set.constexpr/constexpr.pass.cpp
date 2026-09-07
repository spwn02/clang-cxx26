//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <unordered_set>
//
// P3372R3: constexpr containers and adaptors -- unordered_set's member
// surface is constexpr for integral/enum/nullptr_t keys, as long as bucket
// growth stays on a power-of-two trajectory. See docs/CXX26_GAPS.md for the
// boundaries this hits (std::hash's union-based scalar hashing and
// __next_prime being an ABI-exported non-inline function) that are not
// fixed by this change.

#include <unordered_set>

constexpr bool test_unordered_set() {
  std::unordered_set<int> s;
  s.reserve(16); // power-of-two growth avoids __next_prime, see docs/CXX26_GAPS.md
  for (int i = 0; i < 10; ++i)
    s.insert(i);
  if (s.size() != 10)
    return false;

  if (s.find(7) == s.end())
    return false;
  s.erase(7);
  if (s.find(7) != s.end())
    return false;

  std::unordered_set<int> copy(s);
  return copy.size() == s.size();
}
static_assert(test_unordered_set());

// Documented boundary: floating-point keys route through __scalar_hash's
// union type-punning, not constant-evaluable in this compiler.
static_assert(!__builtin_constant_p([] {
  std::unordered_set<double> s;
  s.reserve(4);
  s.insert(1.0);
  return s.size();
}()));

int main(int, char**) { return 0; }
