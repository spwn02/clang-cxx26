//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: no-exceptions

// P3068R6: a library function whose Throws: element applies throws in constant evaluation, and the
// exception can be caught there.

#include <array>
#include <bitset>
#include <cassert>
#include <deque>
#include <format>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#define CHECK_THROWS(name, ...)                                                                                        \
  constexpr bool name() {                                                                                              \
    try {                                                                                                              \
      __VA_ARGS__;                                                                                                     \
      return false;                                                                                                    \
    } catch (...) {                                                                                                    \
      return true;                                                                                                     \
    }                                                                                                                  \
  }                                                                                                                    \
  static_assert(name(), #name);

CHECK_THROWS(vector_at, std::vector<int> v; (void)v.at(3))
CHECK_THROWS(vector_bool_at, std::vector<bool> v; (void)v.at(3))
CHECK_THROWS(vector_reserve, std::vector<int> v; v.reserve(static_cast<std::size_t>(-1)))
CHECK_THROWS(string_at, std::string s("ab"); (void)s.at(9))
CHECK_THROWS(string_substr, std::string s("ab"); (void)s.substr(9))
CHECK_THROWS(string_insert, std::string s("ab"); s.insert(9, "x"))
CHECK_THROWS(string_reserve, std::string s; s.reserve(static_cast<std::size_t>(-1)))
CHECK_THROWS(string_view_at, std::string_view s("ab"); (void)s.at(9))
CHECK_THROWS(array_at, std::array<int, 2> a{}; (void)a.at(9))
CHECK_THROWS(deque_at, std::deque<int> d; (void)d.at(9))
CHECK_THROWS(map_at, std::map<int, int> m; (void)m.at(9))
CHECK_THROWS(bitset_test, std::bitset<4> b; (void)b.test(9))
CHECK_THROWS(bitset_set, std::bitset<4> b; b.set(9))
CHECK_THROWS(bitset_to_ulong, std::bitset<100> b; b.set(99); (void)b.to_ulong())

// The type of the exception is the one of the Throws: element.
constexpr bool exception_types() {
  try {
    std::vector<int> v;
    (void)v.at(1);
  } catch (const std::out_of_range&) {
    try {
      std::vector<int> v;
      v.reserve(static_cast<std::size_t>(-1));
    } catch (const std::length_error&) {
      return true;
    }
  }
  return false;
}
static_assert(exception_types());

int main(int, char**) { return 0; }
