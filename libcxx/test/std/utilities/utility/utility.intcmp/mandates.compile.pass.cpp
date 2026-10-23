//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17

#include <utility>

// The type requirements of [utility.intcmp] are Mandates, not Constraints.
// Invalid types remain viable in an unevaluated, dependent call expression.
template <class T, class U>
concept CanCmpEqual = requires(T t, U u) { std::cmp_equal(t, u); };

template <class T, class U>
concept CanCmpLess = requires(T t, U u) { std::cmp_less(t, u); };

template <class R, class T>
concept CanCheckRange = requires(T t) { std::in_range<R>(t); };

static_assert(CanCmpEqual<double, int>);
static_assert(CanCmpLess<double, int>);
static_assert(CanCheckRange<double, int>);
