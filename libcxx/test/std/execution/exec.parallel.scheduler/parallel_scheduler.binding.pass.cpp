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
#include <execution>
#include <cassert>
#include <memory>

namespace ex   = std::execution;
namespace repl = ex::parallel_scheduler_replacement;
int queries    = 0;
int calls[2]   = {};
struct backend : repl::parallel_scheduler_backend {
  int index;
  explicit backend(int i) : index(i) {}
  void schedule(repl::receiver_proxy& r, std::span<std::byte>) noexcept override {
    ++calls[index];
    r.set_value();
  }
  void schedule_bulk_chunked(std::size_t n, repl::bulk_item_receiver_proxy& r, std::span<std::byte>) noexcept override {
    r.execute(0, n);
    r.set_value();
  }
  void
  schedule_bulk_unchunked(std::size_t n, repl::bulk_item_receiver_proxy& r, std::span<std::byte>) noexcept override {
    r.execute(0, n);
    r.set_value();
  }
};
std::shared_ptr<repl::parallel_scheduler_backend> repl::query_parallel_scheduler_backend() {
  return std::make_shared<backend>(queries++);
}
struct proxy : repl::receiver_proxy {
  void set_value() noexcept override {}
  void set_stopped() noexcept override {}
  void set_error(std::exception_ptr) noexcept override {}
  bool __query_env(const std::type_info&, const std::type_info&, const void*, void*) const noexcept override {
    return false;
  }
};
int main(int, char**) {
  // [exec.parschedrepl.recvproxy]: "template<class P, class-type Query>
  // optional<P> try_query(Query q) const noexcept;"
  const proxy p;
  assert(!p.try_query<std::inplace_stop_token>(std::get_stop_token));
  // [exec.par.scheduler]: "Let eb be the result of
  // parallel_scheduler_replacement::query_parallel_scheduler_backend().
  // If eb == nullptr is true, calls terminate. Otherwise, returns a
  // parallel_scheduler object associated with eb."
  auto a = ex::get_parallel_scheduler();
  auto b = ex::get_parallel_scheduler();
  assert(queries == 2);
  // [exec.par.scheduler]: "Two objects sch and sch2 compare equal if and only if
  // BACKEND-OF(sch) and BACKEND-OF(sch2) refer to the same object."
  assert(a == a && a != b);
  auto copy = a;
  assert(copy == a);
  auto sender = ex::schedule(a);
  assert(ex::get_completion_scheduler<ex::set_value_t>(ex::get_env(sender)) == a);
  auto first  = std::this_thread::sync_wait(std::move(sender));
  auto second = std::this_thread::sync_wait(ex::schedule(b));
  assert(first && second);
  assert(queries == 2 && calls[0] == 1 && calls[1] == 1);
}
