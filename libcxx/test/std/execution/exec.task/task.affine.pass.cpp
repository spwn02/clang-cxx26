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

// [exec.task.promise]: task::promise_type::await_transform returns as_awaitable(sndr, *this) if the start scheduler is an
// inline_scheduler and as_awaitable(affine(sndr), *this) otherwise: a sender that is its own affine (just, read_env, ...)
// takes no scheduling hop, any other one completes through a schedule operation of the start scheduler. The start
// scheduler has to be an infallible-scheduler of the environment of the promise.

#include <cassert>
#include <concepts>
#include <coroutine>
#include <execution>
#include <thread>

namespace ex = std::execution;

// a scheduler that counts the schedule operations started on it (all instances share the counter)
inline int g_count = 0;

struct CountingScheduler {
  using scheduler_concept = ex::scheduler_tag;
  struct Sender {
    using sender_concept = ex::sender_tag;
    ex::env<> get_env() const noexcept { return {}; }
    template <class Self, class... Env>
    static consteval auto get_completion_signatures() {
      return ex::completion_signatures<ex::set_value_t()>{};
    }
    template <class Rcvr>
    struct Op {
      using operation_state_concept = ex::operation_state_tag;
      Rcvr rcvr;
      void start() & noexcept {
        ++g_count;
        ex::set_value(std::move(rcvr));
      }
    };
    template <class Rcvr>
    Op<std::remove_cvref_t<Rcvr>> connect(Rcvr&& r) const noexcept {
      return {std::forward<Rcvr>(r)};
    }
  };
  Sender schedule() const noexcept { return {}; }
  ex::forward_progress_guarantee query(ex::get_forward_progress_guarantee_t) const noexcept {
    return ex::forward_progress_guarantee::weakly_parallel;
  }
  friend bool operator==(CountingScheduler, CountingScheduler) = default;
};
static_assert(ex::scheduler<CountingScheduler>);

struct CountingEnv {
  using start_scheduler_type = CountingScheduler;
};

// a sender without an affine member
struct Plain {
  using sender_concept = ex::sender_tag;
  ex::env<> get_env() const noexcept { return {}; }
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int)>{};
  }
  template <class Rcvr>
  struct Op {
    using operation_state_concept = ex::operation_state_tag;
    Rcvr rcvr;
    void start() & noexcept { ex::set_value(std::move(rcvr), 5); }
  };
  template <class Rcvr>
  Op<std::remove_cvref_t<Rcvr>> connect(Rcvr&& r) const noexcept {
    return {std::forward<Rcvr>(r)};
  }
};

ex::task<int, CountingEnv> affine_sender() {
  co_await ex::just();
  co_await ex::just(3);
  co_return 1;
}

ex::task<int, CountingEnv> plain_sender() {
  int v = co_await Plain{};
  co_return v;
}

ex::task<int, CountingEnv> plain_then() {
  int v = co_await ex::then(ex::just(1), [](int x) { return x + 1; });
  co_return v;
}

ex::task<int> default_environment() {
  int v = co_await Plain{};
  int w = co_await ex::then(ex::just(v), [](int x) { return x * 2; });
  co_return w;
}

// the default start scheduler is a task_scheduler, which is infallible in the environment of a task
using PromiseEnv = ex::env_of_t<ex::task<int>::promise_type>;
static_assert(ex::__infallible_scheduler<ex::task_scheduler, PromiseEnv>);
static_assert(ex::__infallible_scheduler<ex::inline_scheduler, PromiseEnv>);
static_assert(ex::__infallible_scheduler<CountingScheduler, PromiseEnv>);

// a fallible one is not
struct FallibleScheduler : CountingScheduler {
  struct Sender2 : CountingScheduler::Sender {
    template <class Self, class... Env>
    static consteval auto get_completion_signatures() {
      return ex::completion_signatures<ex::set_value_t(), ex::set_error_t(std::exception_ptr)>{};
    }
  };
  Sender2 schedule() const noexcept { return {}; }
};
static_assert(ex::scheduler<FallibleScheduler>);
static_assert(!ex::__infallible_scheduler<FallibleScheduler, PromiseEnv>);

int main(int, char**) {
  // just() is its own affine: no schedule operation of the start scheduler
  {
    g_count = 0;
    auto r  = std::this_thread::sync_wait(affine_sender());
    assert(r && std::get<0>(*r) == 1);
    assert(g_count == 0);
  }
  // a sender without an affine member is continued on the start scheduler: one schedule operation
  {
    g_count = 0;
    auto r  = std::this_thread::sync_wait(plain_sender());
    assert(r && std::get<0>(*r) == 5);
    assert(g_count == 1);
  }
  {
    g_count = 0;
    auto r  = std::this_thread::sync_wait(plain_then());
    assert(r && std::get<0>(*r) == 2);
    assert(g_count == 1);
  }
  // the default environment: the task_scheduler of the task
  {
    auto r = std::this_thread::sync_wait(default_environment());
    assert(r && std::get<0>(*r) == 10);
  }
  return 0;
}
