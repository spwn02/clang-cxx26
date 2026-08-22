//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// [exec.awaitable]: GET-AWAITER, is-awaiter, is-awaitable, await-suspend-result,
// await-result-type, with-await-transform, env-promise. Exposition-only machinery with no
// public surface, so this exercises the private header directly (matching the established
// test/libcxx precedent for exposition-only utilities, e.g. __utility/no_destroy.h's test).

#include <__execution/awaitable.h>
#include <concepts>
#include <coroutine>
#include <type_traits>

#include "test_macros.h"

using namespace std::execution;

// [exec.awaitable]p3: await-suspend-result<T>.
static_assert(__await_suspend_result<void>);
static_assert(__await_suspend_result<bool>);
static_assert(__await_suspend_result<std::coroutine_handle<>>);
static_assert(__await_suspend_result<std::coroutine_handle<int>>);
static_assert(!__await_suspend_result<int>);
static_assert(!__await_suspend_result<void*>);

// A plain, well-formed awaiter: no operator co_await needed, GET-AWAITER's identity
// fallback applies directly.
struct SimpleAwaiter {
  bool await_ready() noexcept { return false; }
  bool await_suspend(std::coroutine_handle<>) noexcept { return false; }
  int await_resume() noexcept { return 42; }
};
static_assert(__is_awaiter<SimpleAwaiter>);
static_assert(__is_awaitable<SimpleAwaiter>);
static_assert(std::is_same_v<__await_result_type<SimpleAwaiter>, int>);

// Not an awaiter and not awaitable: no await_ready/await_suspend/await_resume, no
// operator co_await.
static_assert(!__is_awaiter<int>);
static_assert(!__is_awaitable<int>);

// A type with a member operator co_await.
struct MemberCoAwaitAwaiter {
  bool await_ready() noexcept { return false; }
  bool await_suspend(std::coroutine_handle<>) noexcept { return false; }
  double await_resume() noexcept { return 1.5; }
};
struct HasMemberCoAwait {
  MemberCoAwaitAwaiter operator co_await() && noexcept { return {}; }
};
static_assert(__is_awaitable<HasMemberCoAwait>);
static_assert(std::is_same_v<__await_result_type<HasMemberCoAwait>, double>);

// A type with a free operator co_await, whose awaiter's await_suspend returns a
// coroutine_handle<> (exercising the third await-suspend-result alternative end to end).
struct FreeCoAwaitAwaiter {
  bool await_ready() noexcept { return false; }
  std::coroutine_handle<> await_suspend(std::coroutine_handle<>) noexcept { return {}; }
  void await_resume() noexcept {}
};
struct HasFreeCoAwait {};
FreeCoAwaitAwaiter operator co_await(HasFreeCoAwait) noexcept { return {}; }
static_assert(__is_awaitable<HasFreeCoAwait>);
static_assert(std::is_same_v<__await_result_type<HasFreeCoAwait>, void>);

// A promise with an await_transform member: GET-AWAITER(c, p) routes through
// p.await_transform(c) before the operator co_await step.
struct TransformResultAwaiter {
  bool await_ready() noexcept { return false; }
  bool await_suspend(std::coroutine_handle<>) noexcept { return false; }
  long await_resume() noexcept { return 7; }
};
struct MyValue {};
struct TransformingPromise {
  TransformResultAwaiter await_transform(MyValue) noexcept { return {}; }
};
// MyValue has neither await_ready/suspend/resume nor operator co_await, so it is not
// awaitable on its own -- only through TransformingPromise's await_transform.
static_assert(!__is_awaitable<MyValue>);
static_assert(__is_awaitable<MyValue, TransformingPromise>);
static_assert(std::is_same_v<__await_result_type<MyValue, TransformingPromise>, long>);

// [exec.awaitable]p5: with-await-transform. Identity overload forwards unchanged; the
// has-as-awaitable overload routes through a member .as_awaitable(promise) when present.
struct AsAwaitableAwaiter {
  bool await_ready() noexcept { return false; }
  bool await_suspend(std::coroutine_handle<>) noexcept { return false; }
  int await_resume() noexcept { return 9; }
};
struct HasAsAwaitable {
  template <class _Promise>
  AsAwaitableAwaiter as_awaitable(_Promise&) noexcept {
    return {};
  }
};
struct PlainPromise : __with_await_transform<PlainPromise> {};

static_assert(std::is_same_v<decltype(std::declval<PlainPromise&>().await_transform(std::declval<int&>())), int&>);
static_assert(
    std::is_same_v<decltype(std::declval<PlainPromise&>().await_transform(std::declval<HasAsAwaitable&&>())),
                    AsAwaitableAwaiter>);

// [exec.awaitable]p6: env-promise.
struct MyEnv {
  int tag = 0;
};
static_assert(std::derived_from<__env_promise<MyEnv>, __with_await_transform<__env_promise<MyEnv>>>);
static_assert(std::is_same_v<decltype(std::declval<__env_promise<MyEnv>&>().unhandled_stopped()), std::coroutine_handle<>>);
static_assert(
    std::is_same_v<decltype(std::declval<const __env_promise<MyEnv>&>().get_env()), const MyEnv&>);
// coroutine_handle<__env_promise<Env>> must be a well-formed type (used as GET-AWAITER's
// two-argument form's Promise parameter in [exec.snd.concepts]'s enable-sender).
static_assert(std::is_class_v<std::coroutine_handle<__env_promise<MyEnv>>>);

int main(int, char**) { return 0; }
