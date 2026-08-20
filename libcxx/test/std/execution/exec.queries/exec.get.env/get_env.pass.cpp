//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <execution>

// struct get_env_t {
//   template<class T>
//   constexpr auto operator()(T&&) const noexcept(...) -> decltype(auto);
// };
// inline constexpr get_env_t get_env{};
// template<class T>
// using env_of_t = decltype(execution::get_env(std::declval<T>()));

#include <cassert>
#include <execution>
#include <type_traits>
#include <utility>

struct QueryA {};

struct HasEnv {
  constexpr std::execution::env<std::execution::prop<QueryA, int>> get_env() const noexcept {
    return std::execution::env<std::execution::prop<QueryA, int>>{
        std::execution::prop<QueryA, int>{QueryA{}, 99}};
  }
};

struct NoEnv {};

constexpr bool test() {
  using namespace std::execution;

  // falls back to an empty env<> when there's no get_env() member
  static_assert(std::is_same_v<env_of_t<NoEnv>, env<>>);
  static_assert(noexcept(get_env(NoEnv{})));

  // otherwise forwards to the object's get_env() member
  static_assert(std::is_same_v<env_of_t<HasEnv>, env<prop<QueryA, int>>>);
  assert(get_env(HasEnv{}).query(QueryA{}) == 99);

  return true;
}

int main(int, char**) {
  test();
  static_assert(test());

  return 0;
}
