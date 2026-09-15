//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <memory>

#include <memory>

struct enable_test : std::enable_shared_from_this<enable_test> {
  int value;
  constexpr explicit enable_test(int v) : value(v) {}
};

consteval bool test_shared_ptr() {
  std::shared_ptr<int> p(new int(42));
  std::shared_ptr<int> q(p);
  std::shared_ptr<const int> r(std::move(q));
  if (q || p.use_count() != 2 || *r != 42)
    return false;

  std::weak_ptr<const int> w(r);
  std::shared_ptr<const int> locked = w.lock();
  if (!locked || w.expired() || w.use_count() != 3)
    return false;

  locked.reset();
  r.reset();
  p.reset();
  if (!w.expired() || w.use_count() != 0)
    return false;

  w.reset();
  return true;
}

consteval bool test_assignment_and_enable() {
  std::shared_ptr<int> p(new int(7));
  std::shared_ptr<int> q;
  q = p;
  if (q.use_count() != 2)
    return false;

  std::shared_ptr<int> r;
  r = std::move(q);
  r.swap(p);
  p.reset();
  r.reset();

  auto e = std::make_shared<enable_test>(9);
  auto from_this = e->shared_from_this();
  auto weak = e->weak_from_this();
  return from_this && from_this->value == 9 && !weak.expired() && weak.use_count() == 2;
}

consteval bool test_array_factories() {
  auto bounded = std::make_shared<int[3]>();
  auto unbounded = std::make_shared<int[]>(3);
  bounded[1] = 4;
  unbounded[2] = 8;
  return bounded[1] == 4 && unbounded[2] == 8 && bounded.use_count() == 1 && unbounded.use_count() == 1;
}

static_assert(test_shared_ptr());
static_assert(test_assignment_and_enable());
static_assert(test_array_factories());

int main(int, char**) { return !(test_shared_ptr() && test_assignment_and_enable() && test_array_factories()); }
