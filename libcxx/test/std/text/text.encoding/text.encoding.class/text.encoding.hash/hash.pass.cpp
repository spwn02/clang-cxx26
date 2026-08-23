//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// [text.encoding.hash]
// template<> struct hash<text_encoding>;

#include <text_encoding>

#include <cassert>
#include <type_traits>
#include <unordered_set>

using TE = std::text_encoding;
using ID = TE::id;

static_assert(std::is_default_constructible_v<std::hash<TE>>);
static_assert(std::is_invocable_r_v<std::size_t, std::hash<TE>, const TE&>);

int main(int, char**) {
  std::hash<TE> h;

  // Equal text_encoding objects must hash to the same value.
  TE a(ID::ASCII);
  TE b("US-ASCII");
  assert(a == b);
  assert(h(a) == h(b));

  // Usable as a key in unordered associative containers.
  std::unordered_set<TE> s;
  s.insert(a);
  assert(s.contains(b));

  return 0;
}
