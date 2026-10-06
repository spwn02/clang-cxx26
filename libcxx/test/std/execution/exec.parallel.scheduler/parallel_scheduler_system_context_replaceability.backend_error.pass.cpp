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

// [exec.par.scheduler]: a replacement backend that reports a scheduling error through the receiver proxy: the schedule
// sender has no error completion (the draft contradicts itself here, #263/#270), so the library terminates; it used to
// leave the receiver uncompleted (a hang) unless hardening was enabled.

#include <cassert>
#include <cstdlib>
#include <exception>
#include <execution>
#include <thread>

namespace scr = std::execution::parallel_scheduler_replacement;
namespace ex  = std::execution;

namespace {
class failing_backend final : public scr::parallel_scheduler_backend {
public:
  void schedule(scr::receiver_proxy& proxy, std::span<std::byte>) noexcept override {
    proxy.set_error(std::make_exception_ptr(42));
  }
  void schedule_bulk_chunked(size_t, scr::bulk_item_receiver_proxy& proxy, std::span<std::byte>) noexcept override {
    proxy.set_error(std::make_exception_ptr(42));
  }
  void schedule_bulk_unchunked(size_t, scr::bulk_item_receiver_proxy& proxy, std::span<std::byte>) noexcept override {
    proxy.set_error(std::make_exception_ptr(42));
  }
};
} // namespace

std::shared_ptr<scr::parallel_scheduler_backend> scr::query_parallel_scheduler_backend() {
  static std::shared_ptr<scr::parallel_scheduler_backend> backend = std::make_shared<failing_backend>();
  return backend;
}

int main(int, char**) {
  std::set_terminate([] { std::_Exit(0); }); // the expected outcome
  (void)std::this_thread::sync_wait(ex::schedule(ex::get_parallel_scheduler()));
  std::_Exit(1); // returned: the error was swallowed
}
