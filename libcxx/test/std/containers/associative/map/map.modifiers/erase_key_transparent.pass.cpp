//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// <map>

// P2077R3: heterogeneous erasure
// template<class K> size_type erase(K&& x);

#include <cassert>
#include <map>
#include <string>
#include <string_view>
#include <utility>

struct TransparentComparator {
  using is_transparent = void;
  bool operator()(const std::string& a, const std::string& b) const { return a < b; }
  bool operator()(const std::string& a, std::string_view b) const { return a < b; }
  bool operator()(std::string_view a, const std::string& b) const { return a < b; }
};

struct NonTransparentComparator {bool operator()(const std::string& a, const std::string& b) const { return a < b; }};

template <class M>
concept CanErase = requires(M m) { m.erase(std::string_view("a")); };

static_assert(CanErase<std::map<std::string, int, TransparentComparator>>);
static_assert(!CanErase<const std::map<std::string, int, TransparentComparator>>);
static_assert(!CanErase<std::map<std::string, int, NonTransparentComparator>>);

int main(int, char**) {
  // Erase via a heterogeneous (non-key_type) argument.
  {
    std::map<std::string, int, TransparentComparator> c = {{"a", 1}, {"b", 2}, {"c", 3}};
    std::size_t before = c.size();
    auto n = c.erase(std::string_view("a"));
    assert(n == 1);
    assert(c.size() == before - 1);
  }
  // A key not present: heterogeneous erase returns 0, doesn't throw/UB.
  {
    std::map<std::string, int, TransparentComparator> c = {{"a", 1}, {"b", 2}, {"c", 3}};
    std::size_t before = c.size();
    auto n = c.erase(std::string_view("zz"));
    assert(n == 0);
    assert(c.size() == before);
  }
  return 0;
}
