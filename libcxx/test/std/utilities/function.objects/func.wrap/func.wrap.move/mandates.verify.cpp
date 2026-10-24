//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// A failed static_assert is followed by errors from the instantiation that cannot proceed.
// ADDITIONAL_COMPILE_FLAGS: -Xclang -verify-ignore-unexpected=error,note

// <functional>

// [func.wrap.move.ctor]
// Mandates: is_constructible_v<VT, F> is true.

#include <functional>

struct MoveOnly {
  MoveOnly()                = default;
  MoveOnly(MoveOnly&&)      = default;
  MoveOnly(const MoveOnly&) = delete;
  void operator()() const {}
};

void test() {
  MoveOnly m;
  std::move_only_function<void()> a{m};
  // expected-error@*:* {{the target object must be constructible from the argument}}
  // expected-error@*:* 0+ {{call to deleted constructor}}
}
