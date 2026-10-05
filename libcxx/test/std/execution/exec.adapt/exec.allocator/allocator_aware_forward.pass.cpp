//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <execution>

// [exec.snd.expos]: allocator-aware-forward(obj, context) returns std::forward<T>(obj) if the environment of the context
// has no allocator, and otherwise an object of type remove_cvref_t<T>, also for an lvalue.

#include <cassert>
#include <execution>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

namespace ex = std::execution;

struct Rcvr {
  using receiver_concept = ex::receiver_tag;
  void set_value() && noexcept {}
};

struct AllocRcvr : Rcvr {
  auto get_env() const noexcept { return ex::env(ex::prop(std::get_allocator, std::allocator<int>{})); }
};

void test_allocator_aware_forward() {
  // without an allocator in the environment the object is forwarded as it is
  std::tuple<int> t{7};
  Rcvr plain{};
  static_assert(std::is_same_v<decltype(std::__allocator_aware_forward(t, plain)), std::tuple<int>&>);
  static_assert(std::is_same_v<decltype(std::__allocator_aware_forward(std::move(t), plain)), std::tuple<int>&&>);

  // with one, a new object of the decayed type is made, from an lvalue as well
  AllocRcvr rcvr{};
  auto copy = std::__allocator_aware_forward(t, rcvr);
  static_assert(std::is_same_v<decltype(copy), std::tuple<int>>);
  assert(std::get<0>(copy) == 7);
  const std::tuple<int, int> ct{1, 2};
  auto ccopy = std::__allocator_aware_forward(ct, rcvr);
  static_assert(std::is_same_v<decltype(ccopy), std::tuple<int, int>>);
  assert(std::get<1>(ccopy) == 2);
  auto moved = std::__allocator_aware_forward(std::tuple<int>{9}, rcvr);
  assert(std::get<0>(moved) == 9);

  // not a product: make_obj_using_allocator of the decayed type
  int i = 3;
  auto ic = std::__allocator_aware_forward(i, rcvr);
  static_assert(std::is_same_v<decltype(ic), int>);
  assert(ic == 3);
}

int main(int, char**) {
  test_allocator_aware_forward();
  return 0;
}
