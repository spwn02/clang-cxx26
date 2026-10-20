//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: no-threads

// <rcu>

// [saferecl.rcu.domain.func], [saferecl.rcu.general]: a scheduled evaluation F waits for the end of a
// region of RCU protection R only if the evaluation that scheduled F does not strongly happen before the
// start of R. A thread that schedules an evaluation and then opens a region may call rcu_barrier in it
// (rcu_barrier does not imply rcu_synchronize); a region that started before the scheduling is waited for.

#include <atomic>
#include <cassert>
#include <chrono>
#include <mutex>
#include <rcu>
#include <thread>

struct node : std::rcu_obj_base<node> {
  static inline std::atomic<int> destroyed{0};
  ~node() { ++destroyed; }
};

int main(int, char**) {
  {
    // scheduled before the region: rcu_barrier in the region does not wait for the region
    std::rcu_domain domain;
    (new node)->retire({}, domain);
    domain.lock();
    std::rcu_barrier(domain);
    assert(node::destroyed == 1);
    domain.unlock();
  }

  {
    // nested regions of one thread are closed innermost first
    std::rcu_domain domain;
    domain.lock();
    domain.lock();
    domain.unlock();
    domain.unlock();
    std::rcu_synchronize(domain);
  }

  {
    // a region that started before the scheduling is waited for
    std::rcu_domain domain;
    std::atomic<bool> in_region{false};
    std::atomic<bool> release{false};
    std::thread reader([&] {
      std::scoped_lock lock(domain);
      in_region = true;
      while (!release)
        std::this_thread::yield();
    });
    while (!in_region)
      std::this_thread::yield();
    int before = node::destroyed;
    (new node)->retire({}, domain);
    std::atomic<bool> done{false};
    std::thread barrier([&] {
      std::rcu_barrier(domain);
      done = true;
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    assert(!done);
    assert(node::destroyed == before);
    release = true;
    barrier.join();
    reader.join();
    assert(done);
    assert(node::destroyed == before + 1);
  }
  return 0;
}
