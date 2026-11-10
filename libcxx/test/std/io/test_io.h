//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef TEST_STD_IO_TEST_IO_H
#define TEST_STD_IO_TEST_IO_H

#include <cassert>
#include <coroutine>
#include <cstddef>
#include <exception>
#include <io>
#include <memory_resource>
#include <new>
#include <optional>
#include <stop_token>
#include <utility>

#include "test_macros.h"

namespace test_io {

namespace io = std::io;

struct test_executor;

// A single-threaded run loop: posted continuations are resumed by run(), in FIFO order.
struct test_context : io::execution_context {
  using executor_type = test_executor;

  io::continuation* head = nullptr;
  io::continuation* tail = nullptr;
  bool running           = false;
  int posted             = 0;
  int dispatched_inline  = 0;
  int dispatched_queued  = 0;
  int work_started       = 0;
  int work_finished      = 0;

  ~test_context() {
    shutdown();
    destroy();
  }

  test_executor get_executor() noexcept;

  void enqueue(io::continuation& c) {
    c.next = nullptr;
    if (tail)
      tail->next = &c;
    else
      head = &c;
    tail = &c;
  }

  void run() {
    running = true;
    while (head) {
      io::continuation* c = head;
      head                = c->next;
      if (!head)
        tail = nullptr;
      io::safe_resume(c->h);
    }
    running = false;
  }
};

struct test_executor {
  test_context* ctx;

  friend bool operator==(const test_executor&, const test_executor&) noexcept = default;

  test_context& context() const noexcept { return *ctx; }
  void on_work_started() const noexcept { ++ctx->work_started; }
  void on_work_finished() const noexcept { ++ctx->work_finished; }
  std::coroutine_handle<> dispatch(io::continuation& c) const {
    if (ctx->running) {
      ++ctx->dispatched_inline;
      return c.h;
    }
    ++ctx->dispatched_queued;
    ctx->enqueue(c);
    return std::noop_coroutine();
  }
  void post(io::continuation& c) const {
    ++ctx->posted;
    ctx->enqueue(c);
  }
};

inline test_executor test_context::get_executor() noexcept { return test_executor{this}; }

struct counting_resource : std::pmr::memory_resource {
  int allocations   = 0;
  int deallocations = 0;

  void* do_allocate(std::size_t n, std::size_t) override {
    ++allocations;
    return ::operator new(n);
  }
  void do_deallocate(void* p, std::size_t, std::size_t) override {
    ++deallocations;
    ::operator delete(p);
  }
  bool do_is_equal(const std::pmr::memory_resource& o) const noexcept override { return this == &o; }
};

// Leaf IoAwaitables.

// Resumes through dispatch(): inline when already on the run loop (symmetric transfer).
struct dispatch_op {
  io::continuation c;
  bool await_ready() const noexcept { return false; }
  std::coroutine_handle<> await_suspend(std::coroutine_handle<> h, const io::io_env* env) {
    c.h = h;
    return env->executor.dispatch(c);
  }
  void await_resume() const noexcept {}
};

// Resumes through post(): always deferred.
struct post_op {
  io::continuation c;
  bool await_ready() const noexcept { return false; }
  void await_suspend(std::coroutine_handle<> h, const io::io_env* env) {
    c.h = h;
    env->executor.post(c);
  }
  void await_resume() const noexcept {}
};

// Completes synchronously with the stop state seen through the environment.
struct stop_query_op {
  const io::io_env* env = nullptr;
  bool await_ready() const noexcept { return false; }
  bool await_suspend(std::coroutine_handle<>, const io::io_env* e) noexcept {
    env = e;
    return false; // do not suspend
  }
  bool await_resume() const noexcept { return env->stop_token.stop_requested(); }
};

// A task satisfying IoAwaitable and IoRunnable. Frames come from the cached frame allocator slot.
template <class T>
struct task;

namespace detail {
template <class T>
struct result_storage {
  std::optional<T> value;
  template <class U>
  void return_value(U&& u) {
    value.emplace(std::forward<U>(u));
  }
  T& result() { return *value; }
};
template <>
struct result_storage<void> {
  void return_void() noexcept {}
};
} // namespace detail

template <class T>
struct task {
  struct promise_type : detail::result_storage<T> {
    std::coroutine_handle<> continuation_ = std::noop_coroutine();
    const io::io_env* env_                = nullptr;
    std::exception_ptr ep_;

    // Frame layout: [memory_resource*][padding][frame].
    static constexpr std::size_t header = alignof(std::max_align_t);
    static void* operator new(std::size_t n) {
      std::pmr::memory_resource* mr                = io::get_cached_frame_allocator();
      void* p                                      = mr ? mr->allocate(n + header) : ::operator new(n + header);
      *static_cast<std::pmr::memory_resource**>(p) = mr;
      return static_cast<char*>(p) + header;
    }
    static void operator delete(void* frame, std::size_t n) {
      void* p                       = static_cast<char*>(frame) - header;
      std::pmr::memory_resource* mr = *static_cast<std::pmr::memory_resource**>(p);
      if (mr)
        mr->deallocate(p, n + header);
      else
        ::operator delete(p);
    }

    task get_return_object() { return task{std::coroutine_handle<promise_type>::from_promise(*this)}; }
    std::suspend_always initial_suspend() noexcept { return {}; }
    struct final_awaiter {
      bool await_ready() noexcept { return false; }
      std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> h) noexcept {
        return h.promise().continuation_;
      }
      void await_resume() noexcept {}
    };
    final_awaiter final_suspend() noexcept { return {}; }
    void unhandled_exception() noexcept { ep_ = std::current_exception(); }

    std::exception_ptr exception() const noexcept { return ep_; }
    void set_continuation(std::coroutine_handle<> h) noexcept { continuation_ = h; }
    void set_environment(const io::io_env* e) noexcept { env_ = e; }

    // Injects the environment: the two-argument await_suspend is the protocol boundary.
    template <io::IoAwaitable A>
    auto await_transform(A&& a) {
      struct awaiter {
        A& a_;
        const io::io_env* env_;
        bool await_ready() { return false; }
        auto await_suspend(std::coroutine_handle<> h) { return a_.await_suspend(h, env_); }
        auto await_resume() {
          // Every resume refreshes the out-of-band slot (P4172R1 8.3).
          io::set_cached_frame_allocator(env_->frame_allocator);
          return a_.await_resume();
        }
      };
      return awaiter{a, env_};
    }
  };

  std::coroutine_handle<promise_type> h_;

  explicit task(std::coroutine_handle<promise_type> h) noexcept : h_(h) {}
  task(task&& o) noexcept : h_(std::exchange(o.h_, {})) {}
  ~task() {
    if (h_)
      h_.destroy();
  }

  std::coroutine_handle<promise_type> handle() const noexcept { return h_; }
  std::coroutine_handle<promise_type> release() noexcept { return std::exchange(h_, {}); }

  bool await_ready() const noexcept { return false; }
  std::coroutine_handle<> await_suspend(std::coroutine_handle<> cont, const io::io_env* env) noexcept {
    h_.promise().set_continuation(cont);
    h_.promise().set_environment(env);
    return h_;
  }
  decltype(auto) await_resume() {
    if (h_.promise().ep_)
      std::rethrow_exception(h_.promise().ep_);
    if constexpr (!std::is_void_v<T>)
      return std::move(h_.promise().result());
  }
};

// Runs a root task to completion on the context, with the given stop token and frame allocator.
template <class T>
void run_to_completion(
    test_context& ctx, task<T>& t, std::stop_token st = {}, std::pmr::memory_resource* frame_mr = nullptr) {
  io::io_env env{ctx.get_executor(), st, frame_mr};
  t.handle().promise().set_environment(&env);
  io::continuation start{t.handle()};
  ctx.get_executor().post(start);
  ctx.run();
  assert(t.handle().done());
}

} // namespace test_io

#endif // TEST_STD_IO_TEST_IO_H
