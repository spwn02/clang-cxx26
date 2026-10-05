//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// <memory>

// [unique.ptr.special], P2273R3: the comparison operators of unique_ptr are constexpr since C++23.
//
// template<class T1, class D1, class T2, class D2>
//   constexpr bool operator==(const unique_ptr<T1, D1>&, const unique_ptr<T2, D2>&);
// ... operator<, operator>, operator<=, operator>=, operator<=>, and the overloads taking nullptr_t.

#include <cassert>
#include <compare>
#include <memory>

#include "test_macros.h"

struct Base {};
struct Derived : Base {};

constexpr bool test() {
  {
    std::unique_ptr<int> a, b;
    assert(a == b && !(a != b));
    assert(!(a < b) && !(a > b) && a <= b && a >= b);
    assert((a <=> b) == std::strong_ordering::equal);
  }
  {
    // the relational operators of a null and a non-null pointer are not constant expressions, equality is
    std::unique_ptr<int> null;
    std::unique_ptr<int> live(new int(1));
    assert(null != live && !(null == live));
  }
  {
    std::unique_ptr<Derived> d;
    std::unique_ptr<Base> b;
    assert(d == b);
    assert(!(d < b) && d <= b && d >= b && !(d > b));
    assert((d <=> b) == std::strong_ordering::equal);
  }
  {
    std::unique_ptr<int> p;
    assert(p == nullptr && nullptr == p);
    assert(!(p < nullptr) && !(nullptr < p) && p <= nullptr && nullptr >= p);
    assert((p <=> nullptr) == std::strong_ordering::equal);
    std::unique_ptr<int> q(new int(3));
    assert(q != nullptr && nullptr != q);
  }
  return true;
}

int main(int, char**) {
  assert(test());
#if TEST_STD_VER >= 23
  static_assert(test());
#endif
  return 0;
}
