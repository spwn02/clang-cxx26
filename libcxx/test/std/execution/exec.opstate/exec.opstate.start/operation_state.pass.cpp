//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <execution>

// template<class O>
//   concept operation_state = ...;
// struct start_t { ... };
// inline constexpr start_t start{};

#include <cassert>
#include <execution>

using namespace std::execution;

struct MyOpState {
  using operation_state_concept = operation_state_tag;
  int* started;
  void start() & noexcept { ++*started; }
};
static_assert(operation_state<MyOpState>);

static_assert(!operation_state<int>);

struct MissingTag {
  void start() & noexcept {}
};
static_assert(!operation_state<MissingTag>);

int main(int, char**) {
  int started = 0;
  MyOpState op{&started};
  start(op);
  assert(started == 1);
  static_assert(noexcept(start(op)));

  return 0;
}
