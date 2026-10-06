//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: no-threads

// <execution>

// The completion domain of the adaptors is now known (adaptors_completion_queries): a bulk over such a sender is
// transformed by the domain of the scheduler it completes on (task_scheduler's ts-domain here). The results must not
// depend on which domain is selected.

#include <atomic>
#include <cassert>
#include <execution>
#include <thread>

namespace ex = std::execution;

template <class Sndr>
void check(Sndr&& sndr, std::atomic<int>& sum) {
  sum    = 0;
  auto r = std::this_thread::sync_wait(std::forward<Sndr>(sndr));
  assert(r.has_value());
  assert(sum == 6); // 1 + 2 + 3
}

int main(int, char**) {
  ex::task_scheduler ts{ex::get_parallel_scheduler()};
  std::atomic<int> sum{0};
  auto with_value = [&](int i, int&) noexcept { sum += i + 1; };
  auto no_value   = [&](int i) noexcept { sum += i + 1; };

  check(ex::bulk(ex::continues_on(ex::just(1), ts), ex::par, 3, with_value), sum);
  check(ex::bulk(ex::let_value(ex::just(1), [ts](int) noexcept { return ex::schedule(ts); }), ex::par, 3, no_value), sum);
  check(ex::bulk(ex::starts_on(ts, ex::just(1)), ex::par, 3, with_value), sum);
  check(ex::starts_on(ex::get_parallel_scheduler(), ex::bulk(ex::on(ts, ex::just(1)), ex::par, 3, with_value)), sum);
  check(ex::bulk(ex::schedule_from(ex::continues_on(ex::just(1), ts)), ex::par, 3, with_value), sum);
  check(ex::bulk_chunked(ex::continues_on(ex::just(1), ts), ex::par, 3,
                         [&](int b, int e, int&) noexcept {
                           for (int i = b; i < e; ++i)
                             sum += i + 1;
                         }),
        sum);
  auto all = std::this_thread::sync_wait(ex::when_all(ex::continues_on(ex::just(1), ts), ex::continues_on(ex::just(2), ts)));
  assert(all.has_value());
  return 0;
}
