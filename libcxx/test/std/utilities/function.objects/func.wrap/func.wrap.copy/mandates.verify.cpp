//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// A failed static_assert is followed by errors from the instantiation that cannot proceed.
// ADDITIONAL_COMPILE_FLAGS: -Xclang -verify-ignore-unexpected=error,note

// <functional>

// [func.wrap.copy.ctor]
// Mandates: is_constructible_v<VT, F> and is_copy_constructible_v<VT> are true.

#include <functional>
#include <utility>

struct MoveOnly {
  MoveOnly()                      = default;
  MoveOnly(MoveOnly&&)            = default;
  MoveOnly(const MoveOnly&)       = delete;
  void operator()() const {}
};

void test() {
  std::copyable_function<void()> a{MoveOnly{}};
  // expected-error@*:* {{the target object must be copy constructible}}
  std::copyable_function<void()> b{std::in_place_type<MoveOnly>};
  // expected-error@*:* {{the target object must be copy constructible}}
  MoveOnly m;
  std::copyable_function<void()> c{m};
  // expected-error@*:* {{the target object must be constructible from the argument}}
  // expected-error@*:* 0+ {{call to deleted constructor}}
}
