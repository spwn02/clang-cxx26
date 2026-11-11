//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// REQUIRES: std-at-least-c++29
// UNSUPPORTED: no-threads
// ADDITIONAL_COMPILE_FLAGS: -fexperimental-library -fno-rtti -fno-exceptions

// <execution>, <io_bridge>

// <execution> (the system context, bulk, task_scheduler) and <io_bridge> need neither typeid nor dynamic_cast: they work
// with -fno-rtti (and -fno-exceptions).

#include <execution>
#include <io_bridge>

#include <atomic>
#include <cassert>
#include <optional>
#include <tuple>

namespace ex = std::execution;

int main(int, char**) {
  // the receiver of the system context answers a stop token query (try_query), and bulk runs on it
  std::atomic<int> sum{0};
  auto work = ex::schedule(ex::get_parallel_scheduler()) | ex::bulk(ex::par, 8, [&](int i) { sum += i; });
  auto r    = std::this_thread::sync_wait(std::move(work));
  assert(r.has_value());
  assert(sum == 28);

  // task_scheduler compares the schedulers it holds (a type tag instead of dynamic_cast)
  ex::task_scheduler a(ex::get_parallel_scheduler());
  ex::task_scheduler b(ex::get_parallel_scheduler());
  ex::task_scheduler c(ex::inline_scheduler{});
  assert(a == b);
  assert(a != c);
  assert(a == ex::get_parallel_scheduler());
  assert(!(c == ex::get_parallel_scheduler()));
  assert(c == ex::inline_scheduler{});
  return 0;
}
