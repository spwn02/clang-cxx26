//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <execution>

// template<class Query, class Value>
// struct prop {
//   Query query_;
//   Value value_;
//   constexpr const Value& query(Query) const noexcept { return value_; }
// };
// template<class Query, class Value>
// prop(Query, Value) -> prop<Query, unwrap_reference_t<Value>>;

#include <cassert>
#include <execution>
#include <functional>
#include <type_traits>

struct SomeQuery {};

constexpr bool test() {
  std::execution::prop<SomeQuery, int> p{SomeQuery{}, 42};
  assert(p.query(SomeQuery{}) == 42);

  // extra trailing arguments are ignored
  assert(p.query(SomeQuery{}, 1, 2, 3) == 42);

  // returns a reference to the stored value, not a copy
  static_assert(std::is_same_v<decltype(p.query(SomeQuery{})), const int&>);

  return true;
}

// CTAD
static_assert(std::is_same_v<decltype(std::execution::prop(SomeQuery{}, 42)), std::execution::prop<SomeQuery, int>>);

// CTAD unwraps std::reference_wrapper
static_assert(std::is_same_v<decltype(std::execution::prop(SomeQuery{}, std::declval<std::reference_wrapper<int>>())),
                              std::execution::prop<SomeQuery, int&>>);

int main(int, char**) {
  test();
  static_assert(test());

  return 0;
}
