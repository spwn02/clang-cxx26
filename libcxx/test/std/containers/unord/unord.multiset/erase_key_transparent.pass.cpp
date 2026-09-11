//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// <unordered_set>

// P2077R3: heterogeneous erasure
// template<class K> size_type erase(K&& x);

#include <cassert>
#include <unordered_set>
#include <string>
#include <string_view>
#include <utility>

struct TransparentHash {
  using is_transparent = void;
  size_t operator()(std::string_view s) const { return std::hash<std::string_view>{}(s); }
};

struct NonTransparentHash {size_t operator()(const std::string& s) const { return std::hash<std::string>{}(s); }};

template <class M>
concept CanErase = requires(M m) { m.erase(std::string_view("a")); };

static_assert(CanErase<std::unordered_multiset<std::string, TransparentHash, std::equal_to<>>>);
static_assert(!CanErase<const std::unordered_multiset<std::string, TransparentHash, std::equal_to<>>>);
static_assert(!CanErase<std::unordered_multiset<std::string, NonTransparentHash>>);

int main(int, char**) {
  // Erase via a heterogeneous (non-key_type) argument.
  {
    std::unordered_multiset<std::string, TransparentHash, std::equal_to<>> c = {"a", "a", "b"};
    std::size_t before = c.size();
    auto n = c.erase(std::string_view("a"));
    assert(n == 2);
    assert(c.size() == before - 2);
  }
  // A key not present: heterogeneous erase returns 0, doesn't throw/UB.
  {
    std::unordered_multiset<std::string, TransparentHash, std::equal_to<>> c = {"a", "a", "b"};
    std::size_t before = c.size();
    auto n = c.erase(std::string_view("zz"));
    assert(n == 0);
    assert(c.size() == before);
  }
  return 0;
}
