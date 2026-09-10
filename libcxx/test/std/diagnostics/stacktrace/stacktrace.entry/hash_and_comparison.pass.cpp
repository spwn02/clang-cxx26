//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <stacktrace>

// class stacktrace_entry;
// friend constexpr bool operator==(const stacktrace_entry&, const stacktrace_entry&) noexcept;
// friend constexpr strong_ordering operator<=>(const stacktrace_entry&, const stacktrace_entry&) noexcept;
// template<> struct hash<stacktrace_entry>;

#include <cassert>
#include <compare>
#include <functional>
#include <stacktrace>
#include <unordered_set>

int main(int, char**) {
  std::stacktrace_entry empty1;
  std::stacktrace_entry empty2;
  assert(empty1 == empty2);
  assert((empty1 <=> empty2) == std::strong_ordering::equal);
  assert(std::hash<std::stacktrace_entry>()(empty1) == std::hash<std::stacktrace_entry>()(empty2));

  auto st = std::stacktrace::current();
  assert(!st.empty());
  std::stacktrace_entry real = st[0];
  assert(real != empty1);
  assert((real <=> empty1) != std::strong_ordering::equal);

  // stacktrace_entry must satisfy the Hash/UnorderedAssociativeContainer
  // requirements well enough to be used as a set key.
  std::unordered_set<std::stacktrace_entry> entries;
  for (const auto& e : st)
    entries.insert(e);
  assert(entries.size() == st.size());
  assert(entries.count(st[0]) == 1);
  assert(entries.count(empty1) == 0);

  return 0;
}
