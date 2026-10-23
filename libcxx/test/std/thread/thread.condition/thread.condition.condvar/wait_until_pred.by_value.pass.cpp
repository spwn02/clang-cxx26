//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: no-threads
// UNSUPPORTED: c++03, c++11

// <condition_variable>

// LWG4301: wait_until and wait_for take the time point and the duration by value.
// The predicate changes the caller's time point; the wait must keep using the
// value it was called with, so it times out instead of waiting for an hour.

#include <cassert>
#include <chrono>
#include <condition_variable>
#include <mutex>

int main(int, char**) {
  using namespace std::chrono_literals;
  {
    std::condition_variable cv;
    std::mutex m;
    std::unique_lock<std::mutex> lk(m);
    auto tp     = std::chrono::steady_clock::now() + 50ms;
    auto before = std::chrono::steady_clock::now();
    bool r      = cv.wait_until(lk, tp, [&] {
      tp = std::chrono::steady_clock::now() + 1h;
      return false;
    });
    assert(!r);
    assert(std::chrono::steady_clock::now() - before < 30min);
  }
  {
    std::condition_variable_any cv;
    std::mutex m;
    std::unique_lock<std::mutex> lk(m);
    auto tp     = std::chrono::steady_clock::now() + 50ms;
    auto before = std::chrono::steady_clock::now();
    bool r      = cv.wait_until(lk, tp, [&] {
      tp = std::chrono::steady_clock::now() + 1h;
      return false;
    });
    assert(!r);
    assert(std::chrono::steady_clock::now() - before < 30min);
  }
  return 0;
}
