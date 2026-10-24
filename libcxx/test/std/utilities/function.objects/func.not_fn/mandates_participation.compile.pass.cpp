//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

#include <functional>
#include <utility>

struct NonMovableCallable {
  NonMovableCallable() = default;
  NonMovableCallable(const NonMovableCallable&) = default;
  NonMovableCallable& operator=(const NonMovableCallable&) = default;
  NonMovableCallable(NonMovableCallable&&) = delete;

  bool operator()() const { return true; }
};

template <class F>
concept HasNotFn = requires(F&& f) { std::not_fn(std::forward<F>(f)); };

static_assert(HasNotFn<NonMovableCallable&>);
static_assert(HasNotFn<bool (&)()>);
