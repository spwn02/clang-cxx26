//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17

#include <atomic>
#include <cassert>
#include <memory>
#include <thread>
#include <vector>

#include "test_macros.h"

int main(int, char**) {
  static_assert(std::atomic<std::shared_ptr<int>>::is_always_lock_free == false);
  static_assert(std::atomic<std::weak_ptr<int>>::is_always_lock_free == false);
  static_assert(!std::is_copy_constructible<std::atomic<std::shared_ptr<int>>>::value);

  auto one = std::make_shared<int>(1);
  auto two = std::make_shared<int>(2);
  std::atomic<std::shared_ptr<int>> p(one);
  assert(*p.load() == 1);

  p.store(two, std::memory_order_release);
  assert(*p.load(std::memory_order_acquire) == 2);
  auto old = p.exchange(one);
  assert(*old == 2);
  assert(*p.load() == 1);

  auto expected = two;
  assert(!p.compare_exchange_strong(expected, two));
  assert(expected == one);
  assert(p.compare_exchange_weak(expected, two));
  assert(*p.load() == 2);

  std::weak_ptr<int> weak_one(one);
  std::weak_ptr<int> weak_two(two);
  std::atomic<std::weak_ptr<int>> w(weak_one);
  assert(w.load().lock() == one);
  w.store(weak_two);
  assert(w.load().lock() == two);
  auto weak_expected = weak_one;
  assert(!w.compare_exchange_strong(weak_expected, weak_one));
  assert(weak_expected.lock() == two);

  std::atomic<std::shared_ptr<int>> concurrent(one);
  std::vector<std::thread> workers;
  for (int i = 0; i != 4; ++i) {
    workers.emplace_back([&] {
      for (int n = 0; n != 1000; ++n) {
        auto value = concurrent.load(std::memory_order_acquire);
        assert(value == one || value == two);
        concurrent.store((n & 1) ? one : two, std::memory_order_release);
      }
    });
  }
  for (auto& worker : workers)
    worker.join();

  std::atomic<std::shared_ptr<int>> waited(one);
  std::thread waiter([&] { waited.wait(one); });
  waited.store(two);
  waited.notify_one();
  waiter.join();
}
