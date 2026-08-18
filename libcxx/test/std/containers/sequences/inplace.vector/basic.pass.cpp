//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <inplace_vector>

// Basic functional coverage of std::inplace_vector: capacity enforcement,
// push_back/try_push_back/emplace_back throwing and non-throwing paths,
// insert/erase with element shifting, copy/move, resize, and swap across
// containers of different sizes.

#include <inplace_vector>
#include <cassert>
#include <stdexcept>
#include <utility>

#include "test_macros.h"

struct Counted {
  static int alive;
  int value;
  Counted(int x = 0) : value(x) { ++alive; }
  Counted(const Counted& other) : value(other.value) { ++alive; }
  Counted(Counted&& other) noexcept : value(other.value) { ++alive; }
  Counted& operator=(const Counted&) = default;
  Counted& operator=(Counted&&)      = default;
  ~Counted() { --alive; }
};
int Counted::alive = 0;

int main(int, char**) {
  using IV = std::inplace_vector<int, 4>;
  static_assert(IV::capacity() == 4, "");
  static_assert(IV::max_size() == 4, "");

  {
    IV v;
    assert(v.empty());
    auto __r = v.try_push_back(1);
    assert(__r.has_value() && *__r == 1 && &*__r == &v.back());
    v.push_back(2);
    v.push_back(3);
    v.push_back(4);
    assert(v.size() == 4);
    // try_push_back returns optional<reference>: engaged with the new
    // element on success, disengaged (not comparable to nullptr) on
    // capacity overflow.
    assert(!v.try_push_back(5).has_value());

    bool threw = false;
    try {
      v.push_back(6);
    } catch (const std::bad_alloc&) {
      threw = true;
    }
    assert(threw);

    assert(v[0] == 1 && v[3] == 4);
    v.pop_back();
    assert(v.size() == 3);

    v.insert(v.begin() + 1, 99);
    assert(v[0] == 1 && v[1] == 99 && v[2] == 2 && v[3] == 3);

    v.erase(v.begin() + 1);
    assert(v[0] == 1 && v[1] == 2 && v[2] == 3);

    IV v2 = v;
    assert(v2.size() == v.size());
    assert(v2 == v);

    IV v3(2, 7);
    assert(v3.size() == 2 && v3[0] == 7 && v3[1] == 7);

    v3.resize(4);
    assert(v3.size() == 4);
    v3.resize(1);
    assert(v3.size() == 1);
  }

  {
    IV a{1, 2, 3};
    IV b{9, 8};
    a.swap(b);
    assert(a.size() == 2 && a[0] == 9 && a[1] == 8);
    assert(b.size() == 3 && b[0] == 1 && b[1] == 2 && b[2] == 3);

    auto n = std::erase(a, 9);
    assert(n == 1 && a.size() == 1 && a[0] == 8);
  }

  {
    std::inplace_vector<Counted, 3> cv;
    cv.emplace_back(1);
    cv.emplace_back(2);
    assert(Counted::alive == 2);
    cv.pop_back();
    assert(Counted::alive == 1);
    cv.clear();
    assert(Counted::alive == 0);

    {
      std::inplace_vector<Counted, 3> cv2;
      cv2.emplace_back(5);
      cv2.emplace_back(6);
      std::inplace_vector<Counted, 3> cv3 = std::move(cv2);
      assert(cv3.size() == 2 && cv3[0].value == 5);
    }
    assert(Counted::alive == 0);
  }

  return 0;
}
