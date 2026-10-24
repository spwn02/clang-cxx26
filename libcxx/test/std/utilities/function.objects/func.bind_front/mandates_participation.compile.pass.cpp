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

struct NonMovable {
  NonMovable() = default;
  NonMovable(const NonMovable&) = default;
  NonMovable& operator=(const NonMovable&) = default;
  NonMovable(NonMovable&&) = delete;

  void operator()(int) const {}
};

template <class F>
concept HasBindFrontTarget = requires(F&& f) { std::bind_front(std::forward<F>(f), 1); };

template <class T>
concept HasBindFrontBound = requires(T&& value) { std::bind_front([](int) {}, std::forward<T>(value)); };

static_assert(HasBindFrontTarget<NonMovable&>);
static_assert(HasBindFrontBound<NonMovable&>);
