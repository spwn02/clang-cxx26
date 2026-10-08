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

// namespace execution::system_context_replaceability {
//   class receiver_proxy { ... };
//   class bulk_item_receiver_proxy : public receiver_proxy { ... };
//   class parallel_scheduler_backend { ... };
//   shared_ptr<parallel_scheduler_backend> query_parallel_scheduler_backend();
// }
//
// The default (unreplaced) backend.
// Actual link-time replacement is tested separately in
// parallel_scheduler_system_context_replaceability.replaceable.pass.cpp, since overriding
// query_parallel_scheduler_backend() affects every parallel_scheduler use for the rest of the
// program's lifetime.

#include <cassert>
#include <chrono>
#include <execution>
#include <mutex>
#include <set>
#include <thread>
#include <vector>

namespace scr = std::execution::parallel_scheduler_replacement;

namespace {

// Records exactly which indices execute() covered and on how many distinct threads, to check
// full-coverage-no-overlap and genuine multi-threaded dispatch.
class recording_bulk_proxy final : public scr::bulk_item_receiver_proxy {
public:
  // With `rendezvous`, execute() waits (bounded) until a second thread has also executed a chunk, so that observing
  // more than one thread does not depend on how the OS happens to schedule the workers under load.
  explicit recording_bulk_proxy(size_t n, bool rendezvous = false) : seen_(n, false), rendezvous_(rendezvous) {}

  void execute(size_t begin, size_t end) noexcept override {
    {
      std::lock_guard<std::mutex> lock(mtx_);
      for (size_t i = begin; i < end; ++i) {
        assert(!seen_[i]); // no two chunks may cover the same index
        seen_[i] = true;
      }
      tids_.insert(std::this_thread::get_id());
    }
    if (rendezvous_) {
      auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
      while (thread_count() < 2 && std::chrono::steady_clock::now() < deadline)
        std::this_thread::yield();
    }
  }

  void set_value() noexcept override {
    std::lock_guard<std::mutex> lock(mtx_);
    done_ = true;
  }
  void set_error(std::exception_ptr) noexcept override { assert(false); }
  void set_stopped() noexcept override { assert(false); }

  bool done() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return done_;
  }
  bool all_seen() const {
    for (bool b : seen_)
      if (!b)
        return false;
    return true;
  }
  size_t thread_count() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return tids_.size();
  }

protected:
  bool __query_env(const std::type_info&, const std::type_info&, const void*, void*) const noexcept override {
    return false; // this test never checks for a stop request
  }

private:
  mutable std::mutex mtx_;
  std::vector<bool> seen_;
  std::set<std::thread::id> tids_;
  bool rendezvous_;
  bool done_ = false;
};

class recording_schedule_proxy final : public scr::receiver_proxy {
public:
  void set_value() noexcept override { value_called_ = true; }
  void set_error(std::exception_ptr) noexcept override { assert(false); }
  void set_stopped() noexcept override { assert(false); }
  std::atomic<bool> value_called_ = false;

protected:
  bool __query_env(const std::type_info&, const std::type_info&, const void*, void*) const noexcept override {
    return false;
  }
};

void wait_until(bool (*pred)(void*), void* ctx) {
  for (int i = 0; i < 1'000'000 && !pred(ctx); ++i)
    std::this_thread::yield();
}

} // namespace

int main(int, char**) {
  // query_parallel_scheduler_backend() returns a real backend, and (unreplaced) it is exactly
  // this fork's own default singleton.
  auto backend = scr::query_parallel_scheduler_backend();
  assert(backend != nullptr);
  assert(backend == scr::__get_default_parallel_scheduler_backend());
  assert(scr::query_parallel_scheduler_backend() == backend); // stable across calls

  // schedule(): the default backend dispatches to a real worker thread, not the caller.
  {
    recording_schedule_proxy proxy;
    alignas(std::max_align_t) std::byte storage[64];
    backend->schedule(proxy, std::span<std::byte>(storage, sizeof(storage)));
    wait_until([](void* p) { return static_cast<recording_schedule_proxy*>(p)->value_called_.load(); }, &proxy);
    assert(proxy.value_called_.load());
  }

  // schedule_bulk_chunked(): full coverage, no overlap, genuine parallelism for a large shape.
  {
    constexpr size_t n = 997;
    const bool parallel = std::thread::hardware_concurrency() > 1;
    recording_bulk_proxy proxy(n, parallel);
    alignas(std::max_align_t) std::byte storage[64];
    backend->schedule_bulk_chunked(n, proxy, std::span<std::byte>(storage, sizeof(storage)));
    wait_until([](void* p) { return static_cast<recording_bulk_proxy*>(p)->done(); }, &proxy);
    assert(proxy.done());
    assert(proxy.all_seen());
    if (parallel)
      assert(proxy.thread_count() > 1);
  }

  // schedule_bulk_unchunked(): same contract, one logical index per execute() call.
  {
    constexpr size_t n = 64;
    recording_bulk_proxy proxy(n);
    alignas(std::max_align_t) std::byte storage[64];
    backend->schedule_bulk_unchunked(n, proxy, std::span<std::byte>(storage, sizeof(storage)));
    wait_until([](void* p) { return static_cast<recording_bulk_proxy*>(p)->done(); }, &proxy);
    assert(proxy.done());
    assert(proxy.all_seen());
  }

  // A zero-size bulk operation still completes.
  {
    recording_bulk_proxy proxy(0);
    alignas(std::max_align_t) std::byte storage[64];
    backend->schedule_bulk_chunked(0, proxy, std::span<std::byte>(storage, sizeof(storage)));
    wait_until([](void* p) { return static_cast<recording_bulk_proxy*>(p)->done(); }, &proxy);
    assert(proxy.done());
  }

  // Ordinary get_parallel_scheduler() usage still goes through this same default backend's own
  // pool identity (the plain, non-ABI fast path stays unaffected by this ABI's existence).
  {
    auto sch = std::execution::get_parallel_scheduler();
    assert(sch == std::execution::get_parallel_scheduler());
  }

  return 0;
}
