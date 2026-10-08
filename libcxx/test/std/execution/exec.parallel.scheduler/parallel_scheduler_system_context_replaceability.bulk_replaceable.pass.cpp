//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: no-threads
// UNSUPPORTED: libcpp-has-no-incomplete-pstl

// <execution>
//
// [exec.par.scheduler]: bulk_chunked and bulk_unchunked over a parallel
// scheduler dispatch through the replaceable backend, including its completion
// paths. The replacement is defined with a qualified-id because the facility
// is in libc++'s inline ABI namespace.

#include <cassert>
#include <exception>
#include <execution>
#include <memory>
#include <span>
#include <thread>
#include <vector>

namespace ex  = std::execution;
namespace scr = std::execution::parallel_scheduler_replacement;

namespace {
enum class completion { value, error, stopped };

class recording_backend final : public scr::parallel_scheduler_backend {
public:
  int schedule_calls = 0;
  int chunked_calls = 0;
  int unchunked_calls = 0;
  completion result = completion::value;

  void schedule(scr::receiver_proxy& proxy, std::span<std::byte>) noexcept override {
    ++schedule_calls;
    proxy.set_value();
  }

  void schedule_bulk_chunked(size_t count, scr::bulk_item_receiver_proxy& proxy, std::span<std::byte>) noexcept override {
    ++chunked_calls;
    complete(count, proxy, true);
  }

  void schedule_bulk_unchunked(size_t count, scr::bulk_item_receiver_proxy& proxy, std::span<std::byte>) noexcept override {
    ++unchunked_calls;
    complete(count, proxy, false);
  }

private:
  void complete(size_t count, scr::bulk_item_receiver_proxy& proxy, bool chunked) noexcept {
    if (result == completion::error) {
      proxy.set_error(std::make_exception_ptr(42));
      return;
    }
    if (result == completion::stopped) {
      proxy.set_stopped();
      return;
    }
    if (chunked) {
      if (count != 0)
        proxy.execute(0, count);
    } else {
      for (size_t i = 0; i < count; ++i)
        proxy.execute(i, i + 1);
    }
    proxy.set_value();
  }
};
} // namespace

std::shared_ptr<scr::parallel_scheduler_backend> scr::query_parallel_scheduler_backend() {
  static std::shared_ptr<scr::parallel_scheduler_backend> backend = std::make_shared<recording_backend>();
  return backend;
}

int main(int, char**) {
  auto backend = std::static_pointer_cast<recording_backend>(scr::query_parallel_scheduler_backend());
  auto sch = ex::get_parallel_scheduler();

  {
    std::vector<int> hits(17);
    auto result = std::this_thread::sync_wait(ex::schedule(sch) | ex::bulk_chunked(ex::par, 17, [&](int b, int e) {
                                                for (int i = b; i < e; ++i)
                                                  ++hits[i];
                                              }));
    assert(result.has_value());
    for (int hit : hits)
      assert(hit == 1);
    assert(backend->chunked_calls == 1);
    assert(backend->unchunked_calls == 0);
  }

  {
    std::vector<int> hits(17);
    auto result = std::this_thread::sync_wait(ex::schedule(sch) | ex::bulk_unchunked(ex::par, 17, [&](int i) {
                                                ++hits[i];
                                              }));
    assert(result.has_value());
    for (int hit : hits)
      assert(hit == 1);
    assert(backend->chunked_calls == 1);
    assert(backend->unchunked_calls == 1);
  }

  // bulk composes through bulk_chunked.
  {
    int calls = 0;
    auto result = std::this_thread::sync_wait(ex::schedule(sch) | ex::bulk(ex::par, 5, [&](int) { ++calls; }));
    assert(result.has_value());
    assert(calls == 5);
    assert(backend->chunked_calls == 2);
  }

  // The backend sees zero-size work too and completes it without execute.
  {
    int calls = 0;
    auto result = std::this_thread::sync_wait(ex::schedule(sch) | ex::bulk_unchunked(ex::par, 0, [&](int) { ++calls; }));
    assert(result.has_value());
    assert(calls == 0);
    assert(backend->unchunked_calls == 2);
  }

  // Errors thrown by the bulk function pass through the operation as set_error.
  {
    bool caught = false;
    try {
      (void)std::this_thread::sync_wait(ex::schedule(sch) | ex::bulk_chunked(ex::par, 1, [](int, int) { throw 17; }));
    } catch (int value) {
      caught = value == 17;
    }
    assert(caught);
    assert(backend->chunked_calls == 3);
  }

  // The backend's own error and stop completions must reach the receiver.
  {
    backend->result = completion::error;
    bool caught = false;
    try {
      (void)std::this_thread::sync_wait(ex::schedule(sch) | ex::bulk_unchunked(ex::par, 1, [](int) { assert(false); }));
    } catch (int value) {
      caught = value == 42;
    }
    assert(caught);
    assert(backend->unchunked_calls == 3);
  }
  {
    backend->result = completion::stopped;
    auto result = std::this_thread::sync_wait(ex::schedule(sch) | ex::bulk_chunked(ex::par, 1, [](int, int) { assert(false); }));
    assert(!result.has_value());
    assert(backend->chunked_calls == 4);
  }

  assert(backend->schedule_calls == 7);
  return 0;
}
