//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23, c++26
// UNSUPPORTED: no-threads
// ADDITIONAL_COMPILE_FLAGS: -fexperimental-library

// <io>

// Concept satisfaction and rejection: Executor, ExecutionContext, IoAwaitable, IoRunnable (P4003R3 4.2, 4.3, 4.4, 4.6).

#include <io>

#include <coroutine>
#include <memory_resource>

#include "test_io.h"

namespace io = std::io;
using namespace test_io;

// ---- Executor ----
static_assert(io::Executor<test_executor>);

struct no_post {
  test_context* c;
  friend bool operator==(const no_post&, const no_post&) noexcept = default;
  test_context& context() const noexcept { return *c; }
  void on_work_started() const noexcept {}
  void on_work_finished() const noexcept {}
  std::coroutine_handle<> dispatch(io::continuation&) const { return {}; }
};
static_assert(!io::Executor<no_post>);

struct dispatch_returns_void {
  test_context* c;
  friend bool operator==(const dispatch_returns_void&, const dispatch_returns_void&) noexcept = default;
  test_context& context() const noexcept { return *c; }
  void on_work_started() const noexcept {}
  void on_work_finished() const noexcept {}
  void dispatch(io::continuation&) const {}
  void post(io::continuation&) const {}
};
static_assert(!io::Executor<dispatch_returns_void>);

struct context_by_pointer {
  test_context* c;
  friend bool operator==(const context_by_pointer&, const context_by_pointer&) noexcept = default;
  io::execution_context* context() const noexcept { return c; } // pointer, not an lvalue reference
  void on_work_started() const noexcept {}
  void on_work_finished() const noexcept {}
  std::coroutine_handle<> dispatch(io::continuation&) const { return {}; }
  void post(io::continuation&) const {}
};
static_assert(!io::Executor<context_by_pointer>);

struct context_unrelated {
  int* c;
  friend bool operator==(const context_unrelated&, const context_unrelated&) noexcept = default;
  int& context() const noexcept { return *c; } // not derived from execution_context
  void on_work_started() const noexcept {}
  void on_work_finished() const noexcept {}
  std::coroutine_handle<> dispatch(io::continuation&) const { return {}; }
  void post(io::continuation&) const {}
};
static_assert(!io::Executor<context_unrelated>);

struct throwing_copy : test_executor {
  throwing_copy(const throwing_copy&) noexcept(false);
};
static_assert(!io::Executor<throwing_copy>);

struct throwing_eq {
  test_context* c;
  bool operator==(const throwing_eq&) const noexcept(false) { return true; }
  test_context& context() const noexcept { return *c; }
  void on_work_started() const noexcept {}
  void on_work_finished() const noexcept {}
  std::coroutine_handle<> dispatch(io::continuation&) const { return {}; }
  void post(io::continuation&) const {}
};
static_assert(!io::Executor<throwing_eq>);

struct throwing_work_started {
  test_context* c;
  friend bool operator==(const throwing_work_started&, const throwing_work_started&) noexcept = default;
  test_context& context() const noexcept { return *c; }
  void on_work_started() const noexcept(false) {}
  void on_work_finished() const noexcept {}
  std::coroutine_handle<> dispatch(io::continuation&) const { return {}; }
  void post(io::continuation&) const {}
};
static_assert(!io::Executor<throwing_work_started>);

static_assert(!io::Executor<int>);
static_assert(!io::Executor<void>);
// executor_ref erases an executor but is not itself one (no operator==, no work counting).
static_assert(!io::Executor<io::executor_ref>);
static_assert(std::is_constructible_v<io::executor_ref, const test_executor&>);
static_assert(!std::is_constructible_v<io::executor_ref, const no_post&>);
static_assert(!std::is_constructible_v<io::executor_ref, int>);

// ---- ExecutionContext ----
static_assert(io::ExecutionContext<test_context>);
static_assert(!io::ExecutionContext<io::execution_context>);
static_assert(!io::ExecutionContext<int>);

struct ctx_without_executor_type : io::execution_context {
  test_executor get_executor() noexcept;
};
static_assert(!io::ExecutionContext<ctx_without_executor_type>);

struct ctx_bad_executor_type : io::execution_context {
  using executor_type = int;
  int get_executor() noexcept;
};
static_assert(!io::ExecutionContext<ctx_bad_executor_type>);

struct ctx_throwing_get_executor : io::execution_context {
  using executor_type = test_executor;
  test_executor get_executor(); // not noexcept
};
static_assert(!io::ExecutionContext<ctx_throwing_get_executor>);

struct ctx_not_derived {
  using executor_type = test_executor;
  test_executor get_executor() noexcept;
};
static_assert(!io::ExecutionContext<ctx_not_derived>);

// ---- IoAwaitable ----
static_assert(io::IoAwaitable<dispatch_op>);
static_assert(io::IoAwaitable<post_op>);
static_assert(io::IoAwaitable<stop_query_op>);
static_assert(io::IoAwaitable<task<void>>);
static_assert(io::IoAwaitable<task<int>>);
// A plain C++20 awaitable (one-argument await_suspend) is not an IoAwaitable: the signature is the model boundary.
static_assert(!io::IoAwaitable<std::suspend_always>);
static_assert(!io::IoAwaitable<std::suspend_never>);
static_assert(!io::IoAwaitable<int>);
struct one_arg_awaitable {
  bool await_ready() const noexcept;
  void await_suspend(std::coroutine_handle<>) const noexcept;
  void await_resume() const noexcept;
};
static_assert(!io::IoAwaitable<one_arg_awaitable>);
struct mutable_env_awaitable {
  void await_suspend(std::coroutine_handle<>, io::io_env*) const; // cannot take io_env const*
};
static_assert(!io::IoAwaitable<mutable_env_awaitable>);
struct two_arg_templated {
  template <class H, class E>
  void await_suspend(H, E) const {}
};
static_assert(io::IoAwaitable<two_arg_templated>);

// ---- IoRunnable ----
static_assert(io::IoRunnable<task<void>>);
static_assert(io::IoRunnable<task<int>>);
static_assert(!io::IoRunnable<dispatch_op>); // no promise_type
static_assert(!io::IoRunnable<int>);

template <bool HasRelease, bool HasResult, bool VoidResume, bool HasSetEnv = true>
struct fake_runnable {
  struct promise_type {
    std::exception_ptr exception() const noexcept { return {}; }
    void set_continuation(std::coroutine_handle<>) noexcept {}
    void set_environment(const io::io_env*) noexcept
      requires HasSetEnv
    {}
    int result()
      requires HasResult
    {
      return 0;
    }
  };
  std::coroutine_handle<promise_type> handle() const noexcept;
  void release() noexcept
    requires HasRelease
  {}
  void await_suspend(std::coroutine_handle<>, const io::io_env*) const;
  auto await_resume() const {
    if constexpr (VoidResume)
      return;
    else
      return 0;
  }
};
static_assert(io::IoRunnable<fake_runnable<true, false, true>>);        // void result: no result() needed
static_assert(io::IoRunnable<fake_runnable<true, true, false>>);        // value result with result()
static_assert(!io::IoRunnable<fake_runnable<true, false, false>>);      // value result without result()
static_assert(!io::IoRunnable<fake_runnable<false, true, true>>);       // no release()
static_assert(!io::IoRunnable<fake_runnable<true, true, true, false>>); // no set_environment()

int main(int, char**) { return 0; }
