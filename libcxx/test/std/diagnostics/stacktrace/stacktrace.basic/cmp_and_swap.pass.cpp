//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <stacktrace>

// friend bool operator==(const basic_stacktrace&, const basic_stacktrace&) noexcept;
// friend strong_ordering operator<=>(const basic_stacktrace&, const basic_stacktrace&) noexcept;
// void swap(basic_stacktrace&) noexcept(...);
// template<class Allocator> void swap(basic_stacktrace<Allocator>&, basic_stacktrace<Allocator>&) noexcept(...);

#include <cassert>
#include <compare>
#include <stacktrace>
#include <utility>

int main(int, char**) {
  std::stacktrace empty1;
  std::stacktrace empty2;
  assert(empty1 == empty2);
  assert((empty1 <=> empty2) == std::strong_ordering::equal);

  std::stacktrace st = std::stacktrace::current();
  assert(!st.empty());
  assert(st == st);
  assert(st != empty1);
  assert((st <=> empty1) != std::strong_ordering::equal);

  std::stacktrace copy = st;
  assert(copy == st);

  // Member swap.
  std::stacktrace a = st;
  std::stacktrace b;
  a.swap(b);
  assert(a.empty());
  assert(b == st);

  // Non-member (ADL) swap, and the two-argument std::swap overload.
  std::stacktrace c = st;
  std::stacktrace d;
  swap(c, d);
  assert(c.empty());
  assert(d == st);

  std::stacktrace e = st;
  std::stacktrace f;
  std::swap(e, f);
  assert(e.empty());
  assert(f == st);

  return 0;
}
