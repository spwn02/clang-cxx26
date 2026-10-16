//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <execution>

// [exec.awaitable]: GET-AWAITER(c, p) is the series of transformations applied to the operand of an await-expression in
// a coroutine with promise p: the promise's await_transform, if it has one, then the operator co_await picked by
// overload resolution among the member and non-member candidates, if any. is-awaitable<C, Promise...> asks whether that
// is well-formed and yields an awaiter.

#include <cassert>
#include <concepts>
#include <coroutine>
#include <execution>
#include <type_traits>

namespace ex = std::execution;

struct Awaiter {
  bool await_ready() noexcept { return true; }
  void await_suspend(std::coroutine_handle<>) noexcept {}
  int await_resume() { return 42; }
};

// the awaiter itself
static_assert(ex::__is_awaitable<Awaiter>);
static_assert(std::same_as<ex::__await_result_type<Awaiter>, int>);

// operator co_await: member only, non-member only
struct MemberCoAwait {
  Awaiter operator co_await() { return {}; }
};
struct FreeCoAwait {};
Awaiter operator co_await(FreeCoAwait) { return {}; }
static_assert(ex::__is_awaitable<MemberCoAwait>);
static_assert(ex::__is_awaitable<FreeCoAwait>);

// both: overload resolution is ambiguous, the expression is ill-formed (so it is not a sender either)
struct BothCoAwait : Awaiter {
  Awaiter operator co_await() { return {}; }
};
Awaiter operator co_await(BothCoAwait) { return {}; }
static_assert(!ex::__is_awaitable<BothCoAwait>);
static_assert(!ex::sender<BothCoAwait>);
static_assert(!ex::sender_in<BothCoAwait>);
static_assert(!ex::sender_in<BothCoAwait, ex::env<>>);
// without the ambiguity it is an awaitable sender through its own await_* members
struct JustAwaiter : Awaiter {};
static_assert(ex::sender_in<JustAwaiter>);

// not an awaiter
struct NotAwaiter {};
static_assert(!ex::__is_awaitable<NotAwaiter>);
struct AwaiterNoResume {
  bool await_ready() noexcept { return true; }
  void await_suspend(std::coroutine_handle<>) noexcept {}
};
static_assert(!ex::__is_awaitable<AwaiterNoResume>);
struct BadSuspend {
  bool await_ready() noexcept { return true; }
  int await_suspend(std::coroutine_handle<>) noexcept { return 0; }
  void await_resume() {}
};
static_assert(!ex::__is_awaitable<BadSuspend>);

// await_transform: a promise that has one must accept the operand, there is no fallback to the operand itself
struct NoAwaitTransform {};
struct AcceptsAwaiter {
  Awaiter await_transform(Awaiter a) { return a; }
};
struct AcceptsInt {
  void await_transform(int);
};
struct Template {
  template <class T>
  T&& await_transform(T&& t) {
    return static_cast<T&&>(t);
  }
};
struct Overloaded {
  Awaiter await_transform(int) { return {}; }
  Awaiter await_transform(Awaiter a) { return a; }
};
static_assert(ex::__is_awaitable<Awaiter, NoAwaitTransform>);
static_assert(ex::__is_awaitable<Awaiter, AcceptsAwaiter>);
static_assert(ex::__is_awaitable<Awaiter, Template>);
static_assert(ex::__is_awaitable<Awaiter, Overloaded>);
static_assert(ex::__is_awaitable<int, Overloaded>); // await_transform(int) returns an awaiter
// a promise whose await_transform cannot take the operand: not awaitable (it does not fall back to the operand)
static_assert(!ex::__is_awaitable<Awaiter, AcceptsInt>);
static_assert(!ex::__is_awaitable<MemberCoAwait, AcceptsInt>);
static_assert(!ex::__is_awaitable<int, AcceptsAwaiter>);

// await_transform first, then operator co_await of its result
struct ToMember {
  MemberCoAwait await_transform(Awaiter) { return {}; }
};
static_assert(ex::__is_awaitable<Awaiter, ToMember>);
static_assert(std::same_as<ex::__await_result_type<Awaiter, ToMember>, int>);

// a promise that cannot be derived from (final) is treated as having no await_transform
struct FinalPromise final {};
static_assert(ex::__is_awaitable<Awaiter, FinalPromise>);

// the environment promise of the sender concepts has an await_transform (from with-await-transform)
static_assert(ex::__is_awaitable<Awaiter, ex::__env_promise<ex::env<>>>);

int main(int, char**) { return 0; }
