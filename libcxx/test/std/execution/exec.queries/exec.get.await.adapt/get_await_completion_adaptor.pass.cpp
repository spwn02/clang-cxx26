//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <execution>

// [exec.get.await.adapt]: get_await_completion_adaptor(env) is MANDATE-NOTHROW(AS-CONST(env).query(
// get_await_completion_adaptor)), and a forwarding query.
// [exec.as.awaitable]: adapt-for-await-completion(s) is get_await_completion_adaptor(get_env(s))(s) if that is well-formed
// and s otherwise; as_awaitable(expr, p) uses it on transform_sender(expr, get_env(p)).

#include <cassert>
#include <concepts>
#include <coroutine>
#include <execution>
#include <type_traits>

namespace ex = std::execution;

// ---------------------------------------------------------------------------------------------------------------
// the query object
static_assert(std::is_class_v<ex::get_await_completion_adaptor_t>);
static_assert(std::forwarding_query(ex::get_await_completion_adaptor));

struct Adaptor {
  template <class S>
  constexpr int operator()(S&&) const noexcept {
    return 7;
  }
};
struct WithAdaptor {
  constexpr Adaptor query(ex::get_await_completion_adaptor_t) const noexcept { return {}; }
};
static_assert(std::same_as<decltype(ex::get_await_completion_adaptor(WithAdaptor{})), Adaptor>);
static_assert(ex::get_await_completion_adaptor(WithAdaptor{})(0) == 7);
static_assert(noexcept(ex::get_await_completion_adaptor(WithAdaptor{})));

template <class Env>
concept has_adaptor = requires(const Env& e) { ex::get_await_completion_adaptor(e); };
static_assert(has_adaptor<WithAdaptor>);
static_assert(!has_adaptor<ex::env<>>);
static_assert(!has_adaptor<int>);

// the reference of the query is kept
struct ReferenceEnv {
  Adaptor adaptor;
  const Adaptor& query(ex::get_await_completion_adaptor_t) const noexcept { return adaptor; }
};
static_assert(std::same_as<decltype(ex::get_await_completion_adaptor(std::declval<ReferenceEnv>())), const Adaptor&>);

// forwarded by FWD-ENV, like any forwarding query
static_assert(has_adaptor<decltype(ex::__fwd_env_fn(WithAdaptor{}))>);

// ---------------------------------------------------------------------------------------------------------------
// as_awaitable: the adaptor of the attributes of the sender is applied before the sender becomes awaitable
struct Awaiter {
  bool await_ready() noexcept { return true; }
  void await_suspend(std::coroutine_handle<>) noexcept {}
  int await_resume() { return 42; }
};

struct Op {
  using operation_state_concept = ex::operation_state_tag;
  void start() & noexcept {}
};

// what the adaptor produces: a sender that is awaitable by itself
struct Adapted {
  using sender_concept = ex::sender_tag;
  ex::env<> get_env() const noexcept { return {}; }
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int)>{};
  }
  template <class R>
  Op connect(R&&) && noexcept {
    return {};
  }
  template <class P>
  Awaiter as_awaitable(P&) const noexcept {
    return {};
  }
};
struct MakeAdapted {
  template <class S>
  Adapted operator()(S&&) const noexcept {
    return {};
  }
};
struct AdaptingAttrs {
  MakeAdapted query(ex::get_await_completion_adaptor_t) const noexcept { return {}; }
};
struct AdaptedSender {
  using sender_concept = ex::sender_tag;
  template <class R>
  Op connect(R&&) && noexcept {
    return {};
  }
  AdaptingAttrs get_env() const noexcept { return {}; }
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int)>{};
  }
};
struct PlainSender {
  using sender_concept = ex::sender_tag;
  template <class R>
  Op connect(R&&) && noexcept {
    return {};
  }
  ex::env<> get_env() const noexcept { return {}; }
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int)>{};
  }
};

using Promise = ex::__env_promise<ex::env<>>;

// (7.2): the adapted sender has as_awaitable(p)
static_assert(std::same_as<decltype(ex::as_awaitable(std::declval<AdaptedSender>(), std::declval<Promise&>())), Awaiter>);
// (7.4): without an adaptor the sender is awaited through sender-awaitable
static_assert(!std::same_as<decltype(ex::as_awaitable(std::declval<PlainSender>(), std::declval<Promise&>())), Awaiter>);
static_assert(std::same_as<decltype(ex::as_awaitable(std::declval<PlainSender>(), std::declval<Promise&>())),
                           ex::__sender_awaitable<PlainSender, Promise>>);

// the adaptor result is awaited when it only is a sender (7.4)
struct AdaptedToSenderOnly {
  using sender_concept = ex::sender_tag;
  template <class R>
  Op connect(R&&) && noexcept {
    return {};
  }
  ex::env<> get_env() const noexcept { return {}; }
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int)>{};
  }
};
struct MakeSenderOnly {
  template <class S>
  AdaptedToSenderOnly operator()(S&&) const noexcept {
    return {};
  }
};
struct SenderOnlyAttrs {
  MakeSenderOnly query(ex::get_await_completion_adaptor_t) const noexcept { return {}; }
};
struct SenderOnly {
  using sender_concept = ex::sender_tag;
  template <class R>
  Op connect(R&&) && noexcept {
    return {};
  }
  SenderOnlyAttrs get_env() const noexcept { return {}; }
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int)>{};
  }
};
static_assert(std::same_as<decltype(ex::as_awaitable(std::declval<SenderOnly>(), std::declval<Promise&>())),
                           ex::__sender_awaitable<AdaptedToSenderOnly, Promise>>);

int main(int, char**) { return 0; }
