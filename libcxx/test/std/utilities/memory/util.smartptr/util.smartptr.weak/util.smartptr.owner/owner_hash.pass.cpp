//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <memory>

// struct owner_hash {
//     template<class T>
//         size_t operator()(const shared_ptr<T>&) const noexcept;
//     template<class T>
//         size_t operator()(const weak_ptr<T>&) const noexcept;
//
//     using is_transparent = unspecified;
// };

#include <cassert>
#include <memory>
#include <type_traits>
#include <unordered_set>

#include "test_macros.h"

struct X {};

int main(int, char**) {
  const std::shared_ptr<int> p1(new int);
  const std::shared_ptr<int> p2 = p1;
  const std::weak_ptr<int> w1(p1);

  std::owner_hash oh;

  using IsTransparent = std::owner_hash::is_transparent; // just needs to exist
  static_assert(std::is_same<IsTransparent, IsTransparent>::value, "");

  assert(oh(p1) == oh(p2));
  assert(oh(p1) == oh(w1));

  ASSERT_NOEXCEPT(oh(p1));
  ASSERT_NOEXCEPT(oh(w1));
  ASSERT_SAME_TYPE(decltype(oh(p1)), std::size_t);

  {
    // heterogeneous lookup between shared_ptr and weak_ptr keys
    std::unordered_set<std::shared_ptr<X>, std::owner_hash, std::owner_equal> s;
    std::shared_ptr<X> sx = std::make_shared<X>();
    s.insert(sx);
    std::weak_ptr<X> wx = sx;
    assert(s.find(wx) != s.end());
  }

  return 0;
}
