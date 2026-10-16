//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <execution>

#include <execution>
#include <memory>
#include <type_traits>
#include <cassert>

struct query {
  template <class E> constexpr decltype(auto) operator()(const E& e) const noexcept { return e.query(*this); }
};
int main(int, char**) {
  // [exec.prop]: "QueryTag query_; ValueType value_;"
  // "Specializations of prop are not assignable."
  auto p = std::execution::prop{query{}, std::make_unique<int>(42)};
  static_assert(std::is_aggregate_v<decltype(p)>);
  static_assert(!std::is_move_assignable_v<decltype(p)>);
  static_assert(!std::is_copy_assignable_v<decltype(p)>);
  auto q = std::move(p);
  assert(*q.query(query{}) == 42);
  return 0;
}
