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

// shared_ptr<parallel_scheduler_backend> query_parallel_scheduler_backend();
//
// [exec.sysctx.replaceability]p2 (via the paper's own description): "If parallel scheduler
// backend is replaced, the entire program will only see the replacement, and not the default
// implementation." Test that a program-supplied definition displaces the library's default and
// that ordinary get_parallel_scheduler() usage actually routes through it -- per
// [replacement.functions]-style replaceability, matching how
// debugging.utility/is_debugger_present.replaceable.pass.cpp tests an unrelated replaceable
// function.
//
// IMPORTANT (see <__execution/system_context_replaceability.h>'s own comment on
// query_parallel_scheduler_backend()): the override below MUST be a qualified-id function
// definition, not a `namespace std::execution::system_context_replaceability { ... }` reopen --
// the latter silently creates an unrelated namespace (a real mistake made and caught while
// developing this test) since this facility lives inside libc++'s inline ABI-versioned
// namespace, unlike global-scope replaceable functions such as operator new.

#include <atomic>
#include <cassert>
#include <execution>
#include <thread>

namespace scr = std::execution::system_context_replaceability;

namespace {
std::atomic<int> g_schedule_calls{0};

// Runs everything inline, synchronously, on the calling thread -- the simplest possible correct
// implementation, and observably different from the real pool's genuine worker-thread dispatch.
class inline_backend final : public scr::parallel_scheduler_backend {
public:
  void schedule(scr::receiver_proxy& proxy, std::span<std::byte>) noexcept override {
    g_schedule_calls.fetch_add(1);
    proxy.set_value();
  }
  void schedule_bulk_chunked(size_t count, scr::bulk_item_receiver_proxy& proxy, std::span<std::byte>) noexcept override {
    if (count > 0)
      proxy.execute(0, count);
    proxy.set_value();
  }
  void schedule_bulk_unchunked(size_t count, scr::bulk_item_receiver_proxy& proxy, std::span<std::byte>) noexcept override {
    for (size_t i = 0; i < count; ++i)
      proxy.execute(i, i + 1);
    proxy.set_value();
  }
};
} // namespace

std::shared_ptr<scr::parallel_scheduler_backend> scr::query_parallel_scheduler_backend() {
  static std::shared_ptr<scr::parallel_scheduler_backend> backend = std::make_shared<inline_backend>();
  return backend;
}

int main(int, char**) {
  // The replacement is genuinely active, not shadowed by the default.
  auto active  = scr::query_parallel_scheduler_backend();
  auto default_backend = scr::__get_default_parallel_scheduler_backend();
  assert(active != default_backend);

  // Ordinary parallel_scheduler usage (through get_parallel_scheduler(), not a direct ABI call)
  // now dispatches through the replacement: proven by running entirely on the caller's own
  // thread instead of a genuine worker thread.
  auto sch                        = std::execution::get_parallel_scheduler();
  std::thread::id caller_tid      = std::this_thread::get_id();
  std::thread::id completion_tid  = {};
  bool ran                        = false;
  auto result = std::this_thread::sync_wait(
      std::execution::schedule(sch) | std::execution::then([&] {
        ran            = true;
        completion_tid = std::this_thread::get_id();
      }));
  assert(result.has_value());
  assert(ran);
  assert(completion_tid == caller_tid); // did NOT go through the real worker-thread pool
  assert(g_schedule_calls.load() == 1);

  return 0;
}
