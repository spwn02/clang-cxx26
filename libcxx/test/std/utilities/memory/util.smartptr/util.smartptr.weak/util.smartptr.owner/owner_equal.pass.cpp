//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <memory>

// struct owner_equal {
//     template<class T, class U>
//         bool operator()(const shared_ptr<T>&, const shared_ptr<U>&) const noexcept;
//     template<class T, class U>
//         bool operator()(const shared_ptr<T>&, const weak_ptr<U>&) const noexcept;
//     template<class T, class U>
//         bool operator()(const weak_ptr<T>&, const shared_ptr<U>&) const noexcept;
//     template<class T, class U>
//         bool operator()(const weak_ptr<T>&, const weak_ptr<U>&) const noexcept;
//
//     using is_transparent = unspecified;
// };

#include <cassert>
#include <memory>
#include <type_traits>
#include <unordered_map>

#include "test_macros.h"

struct X {};

int main(int, char**) {
  const std::shared_ptr<int> p1(new int);
  const std::shared_ptr<int> p2 = p1;
  const std::shared_ptr<int> p3(new int);
  const std::weak_ptr<int> w1(p1);
  const std::weak_ptr<int> w3(p3);

  std::owner_equal oe;

  using IsTransparent = std::owner_equal::is_transparent; // just needs to exist
  static_assert(std::is_same<IsTransparent, IsTransparent>::value, "");

  assert(oe(p1, p2));
  assert(!oe(p1, p3));
  assert(oe(p1, w1));
  assert(!oe(p1, w3));
  assert(oe(w1, p1));
  assert(!oe(w1, p3));
  assert(oe(w1, w1));
  assert(!oe(w1, w3));

  ASSERT_NOEXCEPT(oe(p1, p2));
  ASSERT_NOEXCEPT(oe(p1, w1));
  ASSERT_NOEXCEPT(oe(w1, p1));
  ASSERT_NOEXCEPT(oe(w1, w1));
  ASSERT_SAME_TYPE(decltype(oe(p1, p2)), bool);

  {
    // heterogeneous lookup between shared_ptr and weak_ptr keys
    std::unordered_map<std::shared_ptr<X>, int, std::owner_hash, std::owner_equal> m;
    std::shared_ptr<X> sx = std::make_shared<X>();
    m[sx]                 = 42;
    std::weak_ptr<X> wx   = sx;
    auto it               = m.find(wx);
    assert(it != m.end());
    assert(it->second == 42);
  }

  return 0;
}
