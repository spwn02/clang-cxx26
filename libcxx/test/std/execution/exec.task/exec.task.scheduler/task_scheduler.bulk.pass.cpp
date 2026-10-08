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

#include <cassert>
#include <execution>
#include <memory>
#include <span>
#include <thread>

namespace ex  = std::execution;
namespace scr = std::execution::parallel_scheduler_replacement;

int custom_transforms = 0;
struct custom_domain {
  template <class Sender, class Env>
    requires(ex::__sender_for<Sender, ex::bulk_chunked_t> || ex::__sender_for<Sender, ex::bulk_unchunked_t>)
  decltype(auto) transform_sender(ex::set_value_t, Sender&& sender, const Env&) const noexcept {
    ++custom_transforms;
    return std::forward<Sender>(sender);
  }
};

struct custom_scheduler {
  using scheduler_concept = ex::scheduler_tag;
  friend bool operator==(custom_scheduler, custom_scheduler) = default;
  auto query(ex::get_forward_progress_guarantee_t) const noexcept {
    return ex::forward_progress_guarantee::concurrent;
  }
  custom_domain query(ex::get_completion_domain_t<ex::set_value_t>) const noexcept { return {}; }
  custom_scheduler query(ex::get_completion_scheduler_t<ex::set_value_t>) const noexcept { return {}; }
  struct sender {
    using sender_concept = ex::sender_tag;
    struct attrs {
      custom_scheduler query(ex::get_completion_scheduler_t<ex::set_value_t>) const noexcept { return {}; }
      custom_domain query(ex::get_completion_domain_t<ex::set_value_t>) const noexcept { return {}; }
    };
    attrs get_env() const noexcept { return {}; }
    template <class Receiver>
    struct op {
      using operation_state_concept = ex::operation_state_tag;
      Receiver receiver;
      void start() & noexcept { ex::set_value(std::move(receiver)); }
    };
    template <class Receiver>
    auto connect(Receiver&& receiver) const { return op<std::remove_cvref_t<Receiver>>{std::forward<Receiver>(receiver)}; }
    template <class, class...>
    static consteval auto get_completion_signatures() { return ex::completion_signatures<ex::set_value_t()>{}; }
  };
  sender schedule() const noexcept { return {}; }
};

struct recording_backend : scr::parallel_scheduler_backend {
  int chunked = 0;
  int unchunked = 0;
  void schedule(scr::receiver_proxy& r, std::span<std::byte>) noexcept override { r.set_value(); }
  void schedule_bulk_chunked(size_t n, scr::bulk_item_receiver_proxy& r, std::span<std::byte>) noexcept override {
    ++chunked;
    if (n) r.execute(0, n);
    r.set_value();
  }
  void schedule_bulk_unchunked(size_t n, scr::bulk_item_receiver_proxy& r, std::span<std::byte>) noexcept override {
    ++unchunked;
    for (size_t i = 0; i != n; ++i) r.execute(i, i + 1);
    r.set_value();
  }
};

std::shared_ptr<scr::parallel_scheduler_backend> scr::query_parallel_scheduler_backend() {
  static auto backend = std::make_shared<recording_backend>();
  return backend;
}

int main(int, char**) {
  auto backend = std::static_pointer_cast<recording_backend>(scr::query_parallel_scheduler_backend());
  ex::task_scheduler parallel(ex::get_parallel_scheduler());
  int count = 0;
  std::this_thread::sync_wait(ex::schedule(parallel) | ex::bulk_chunked(ex::par, 7, [&](int b, int e) {
                                count += e - b;
                              }));
  assert(count == 7 && backend->chunked == 1);
  std::this_thread::sync_wait(ex::schedule(parallel) | ex::bulk_unchunked(ex::par, 7, [&](int) { ++count; }));
  assert(count == 14 && backend->unchunked == 1);

  // A scheduler without a parallel backend still schedules its predecessor
  // and covers the full shape through the type-erased bulk backend.
  ex::run_loop loop;
  ex::task_scheduler user(loop.get_scheduler());
  int user_count = 0;
  struct receiver {
    using receiver_concept = ex::receiver_tag;
    int* done;
    void set_value() && noexcept { ++*done; }
    void set_error(std::exception_ptr) && noexcept { assert(false); }
    void set_stopped() && noexcept { assert(false); }
    auto get_env() const noexcept { return ex::env<>{}; }
  };
  auto op = ex::connect(ex::schedule(user) | ex::bulk_unchunked(ex::par, 7, [&](int) { ++user_count; }),
                        receiver{&count});
  ex::start(op);
  assert(user_count == 0);
  loop.finish();
  loop.run();
  assert(user_count == 7 && count == 15);

  // Synchronous completion also covers both bulk forms through the fallback.
  ex::task_scheduler inline_user(ex::inline_scheduler{});
  int inline_count = 0;
  std::this_thread::sync_wait(ex::schedule(inline_user) |
                              ex::bulk_chunked(ex::par, 5, [&](int b, int e) { inline_count += e - b; }));
  std::this_thread::sync_wait(ex::schedule(inline_user) |
                              ex::bulk_unchunked(ex::par, 5, [&](int) { ++inline_count; }));
  assert(inline_count == 10);

  ex::task_scheduler customized(custom_scheduler{});
  static_assert(std::same_as<decltype(ex::get_completion_domain<>(ex::get_env(ex::bulk_unchunked(
                                 custom_scheduler{}.schedule(), ex::par, 1, [](int) {})))), custom_domain>);
  int customized_count = 0;
  std::this_thread::sync_wait(ex::schedule(customized) |
                              ex::bulk_unchunked(ex::par, 3, [&](int) { ++customized_count; }));
  assert(customized_count == 3 && custom_transforms > 0);
}
