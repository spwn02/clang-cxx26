//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03

// <__algorithm/unwrap_iter.h>

// A bounded iterator must never be unwrapped to a raw pointer: a memmove-based algorithm would then write through an
// output range before any bounds check could run (#157).

#include <__algorithm/unwrap_iter.h>
#include <__iterator/bounded_iter.h>
#include <__iterator/static_bounded_iter.h>
#include <__iterator/wrap_iter.h>
#include <__type_traits/is_same.h>
#include <__utility/declval.h>

template <class Iter>
void test() {
  using Bounded = std::__bounded_iter<Iter>;
  static_assert(std::is_same<decltype(std::__unwrap_iter(std::declval<Bounded>())), Bounded>::value, "");
  static_assert(std::is_same<decltype(std::__rewrap_iter(std::declval<Bounded>(), std::declval<Bounded>())), Bounded>::value,
                "");
}

template <class Iter>
void test_static() {
  using Bounded = std::__static_bounded_iter<Iter, 4>;
  static_assert(std::is_same<decltype(std::__unwrap_iter(std::declval<Bounded>())), Bounded>::value, "");
  static_assert(std::is_same<decltype(std::__rewrap_iter(std::declval<Bounded>(), std::declval<Bounded>())), Bounded>::value,
                "");
}

void tests() {
  test_static<int*>();
  test_static<const int*>();
  test<int*>();
  test<const int*>();
  test<std::__wrap_iter<int*> >();
  test<std::__wrap_iter<const int*> >();
}
